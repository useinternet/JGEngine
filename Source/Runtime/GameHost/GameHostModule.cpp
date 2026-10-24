#include "PCH/PCH.h"
#include "GameHostModule.h"
#include "Platform/JWindow.h"
#include "Platform/Platform.h"
#include "FileIO/FileHelper.h"
#include "FileIO/ProjectDescriptor.h"
#include "JGGraphics.h"
#include "JGGraphicsAPI.h"
#include "Viewport/GameViewport.h"
#include <shellapi.h>

JG_MODULE_IMPL(HGameHostModule, GAMEHOST_C_API)

namespace
{
	// JGEditor 의 엔진 모듈 순서에서 에디터 · 개발 모듈(GUI)를 뺀 것.
	const char* const GAME_HOST_ENGINE_MODULES[] = { "Graphics", "GameFrameWork" };

	// 창 크기 변경이 이만큼 프레임 동안 멈추면 뷰포트(렌더 텍스처) 크기를 맞춘다(끄는 동안 매 프레임 다시 만들지 않게).
	const int32 RESIZE_SETTLE_FRAMES = 10;

	// 최소화 동안 프레임 루프가 쉴 시간(ms). Present 가 없어 VSync 로도 막히지 않고 끝없이 돈다(Graphics 알림).
	const DWORD MINIMIZED_SLEEP_MS = 15;

	struct HCommandLineWindow
	{
		bool  bWindowed   = false;
		bool  bFullscreen = false;
		bool  bNoVSync    = false;
		int32 Width  = 0;
		int32 Height = 0;
	};

	// 명령줄 창 옵션: -windowed · -fullscreen(테두리 없는 전체 화면) · -res=WxH(클라이언트 크기) · -novsync.
	HCommandLineWindow parseCommandLine()
	{
		HCommandLineWindow result;

		int32 argc = 0;
		LPWSTR* argv = ::CommandLineToArgvW(::GetCommandLineW(), &argc);
		if (argv == nullptr)
		{
			return result;
		}

		const std::wstring resolutionKey = L"-res=";
		for (int32 i = 1; i < argc; ++i)
		{
			const std::wstring argument = argv[i];
			if (argument == L"-windowed")
			{
				result.bWindowed = true;
			}
			else if (argument == L"-fullscreen")
			{
				result.bFullscreen = true;
			}
			else if (argument == L"-novsync")
			{
				result.bNoVSync = true;
			}
			else if (argument.rfind(resolutionKey, 0) == 0)
			{
				const std::wstring value = argument.substr(resolutionKey.size());
				const size_t separator = value.find_first_of(L"xX");
				if (separator != std::wstring::npos)
				{
					result.Width  = _wtoi(value.substr(0, separator).c_str());
					result.Height = _wtoi(value.substr(separator + 1).c_str());
				}
			}
		}

		::LocalFree(argv);
		return result;
	}

	// 창 모드에서 원하는 클라이언트 크기 → 창 바깥 크기(PWindowsJWindow 와 같은 스타일로 계산 — HJWindowArguments::Size 는 바깥 크기).
	// 바깥 크기가 모니터 작업 영역보다 크면 비율을 지켜 줄인다(1080p 모니터에 1920×1080 클라이언트는 제목 줄 · 작업 표시줄 때문에 다 안 들어간다).
	HVector2Int outerSizeForClient(const HVector2Int& requestedClient, bool bResizable, HVector2Int* outClient)
	{
		DWORD style = WS_OVERLAPPEDWINDOW;
		if (bResizable == false)
		{
			style &= ~(WS_THICKFRAME | WS_MAXIMIZEBOX);
		}

		RECT frame = { 0, 0, 0, 0 };
		::AdjustWindowRectEx(&frame, style, FALSE, 0);
		const int32 frameWidth  = frame.right - frame.left;
		const int32 frameHeight = frame.bottom - frame.top;

		RECT workArea = { 0, 0, 0, 0 };
		::SystemParametersInfoW(SPI_GETWORKAREA, 0, &workArea, 0);
		const int32 maxClientWidth  = (workArea.right - workArea.left) - frameWidth;
		const int32 maxClientHeight = (workArea.bottom - workArea.top) - frameHeight;

		HVector2Int client = requestedClient;
		if (maxClientWidth > 0 && maxClientHeight > 0 && (client.x > maxClientWidth || client.y > maxClientHeight))
		{
			const double scaleX = (double)maxClientWidth / (double)client.x;
			const double scaleY = (double)maxClientHeight / (double)client.y;
			const double scale  = (scaleX < scaleY) ? scaleX : scaleY;
			client.x = (int32)(client.x * scale);
			client.y = (int32)(client.y * scale);
			JG_LOG(GameHost, ELogLevel::Info, "GameHost: %dx%d does not fit the work area, using %dx%d", requestedClient.x, requestedClient.y, client.x, client.y);
		}

		*outClient = client;
		return HVector2Int(client.x + frameWidth, client.y + frameHeight);
	}

	PSharedPtr<PJGGraphicsAPI> findGraphicsAPI()
	{
		HJGGraphicsModule* graphicsModule = GModuleGlobalSystem::GetInstance().FindModule<HJGGraphicsModule>();
		return (graphicsModule != nullptr) ? graphicsModule->GetGraphicsAPI() : nullptr;
	}
}

JGType HGameHostModule::GetModuleType() const
{
	return JGTYPE(HGameHostModule);
}

void HGameHostModule::StartupModule()
{
	HList<PString> gameModules;
	if (loadSettings(&gameModules) == false)
	{
		JG_LOG(GameHost, ELogLevel::Critical, "GameHost: no game settings (package descriptor or project) - nothing to run");
		GCoreSystem::RequestExit(3);
		return;
	}

	if (createWindow() == false)
	{
		JG_LOG(GameHost, ELogLevel::Critical, "GameHost: fail to create the game window");
		GCoreSystem::RequestExit(3);
		return;
	}

	connectModules(gameModules);

	// 게임은 VSync 를 기본으로 켠다(설정 Window.VSync · -novsync). 에디터는 Graphics 기본값(끔) 그대로다.
	PSharedPtr<PJGGraphicsAPI> graphicsAPI = findGraphicsAPI();
	if (graphicsAPI != nullptr)
	{
		graphicsAPI->SetVSyncEnabled(_windowSettings.bVSync);
	}

	// 게임 화면 = 게임 인스턴스의 월드(활성 카메라) + 게임 UI. 그래픽이 없으면 뷰포트는 라우팅만 한다(Render 가 false).
	const HVector2Int client = _window->GetClientSize();
	_viewport = Allocate<PGameViewport>();
	_viewport->Initialize(PString("GameHost"), (uint32)client.x, (uint32)client.y);

	HCoreSystemGlobalValues& globalValues = GCoreSystem::GetGlobalValues();
	globalValues.WindowCallBacks->OnResize.AddRaw(this, &HGameHostModule::onResize);
	globalValues.WindowCallBacks->OnInput.AddRaw(this, &HGameHostModule::onInput);

	// GFW 가 Update 단계에 월드 · UI 를 갱신한 뒤, Graphics 가 GraphicsEnd 에 Present 하기 전.
	GScheduleGlobalSystem::GetInstance().ScheduleByFrame(EMainThreadExecutionOrder::End, PTaskDelegate::CreateRaw(this, &HGameHostModule::renderFrame));

	PString moduleList;
	for (const PString& moduleName : _connectedGameModules)
	{
		moduleList.Append(moduleList.Empty() ? "" : ", ").Append(moduleName);
	}
	JG_LOG(GameHost, ELogLevel::Info, "GameHost: started \"%s\" %dx%d %s, VSync %s, game modules: %s", _windowSettings.Title, client.x, client.y, _windowSettings.Mode, _windowSettings.bVSync ? "on" : "off", moduleList);
}

void HGameHostModule::ShutdownModule()
{
	HCoreSystemGlobalValues& globalValues = GCoreSystem::GetGlobalValues();
	if (globalValues.WindowCallBacks != nullptr)
	{
		globalValues.WindowCallBacks->OnResize.RemoveObject(this);
		globalValues.WindowCallBacks->OnInput.RemoveObject(this);
	}

	// 뷰포트는 GPU 텍스처 · 게임 인스턴스를 들고 있으므로 게임 모듈 · GameFrameWork · Graphics 보다 먼저 놓는다.
	_viewport = nullptr;

	// 게임 모듈은 엔진 모듈을 쓰므로 먼저 내린다(JGEditor 와 같은 규칙).
	while (_connectedGameModules.empty() == false)
	{
		GModuleGlobalSystem::GetInstance().DisconnectModule(_connectedGameModules.back());
		_connectedGameModules.pop_back();
	}

	_window = nullptr;

	while (_connectedEngineModules.empty() == false)
	{
		GModuleGlobalSystem::GetInstance().DisconnectModule(_connectedEngineModules.back());
		_connectedEngineModules.pop_back();
	}

	JG_LOG(GameHost, ELogLevel::Info, "GameHost: shutdown");
}

bool HGameHostModule::loadSettings(HList<PString>* outGameModules)
{
#if JG_WITH_EDITOR
	// 개발 실행(JGLauncher -game): .jgproject. 엔진 단독이면 게임 모듈 없이 빈 화면(창 · 출력 경로 확인용).
	_windowSettings = HGameWindowSettings();
	if (HFileHelper::IsProjectMode() == true)
	{
		HProjectDescriptor descriptor;
		if (HProjectDescriptor::Load(HFileHelper::ProjectDirectory(), &descriptor) == false)
		{
			return false;
		}

		*outGameModules = descriptor.GameModules;
		_windowSettings = descriptor.Package.Window;
		if (_windowSettings.Title.Empty() == true)
		{
			_windowSettings.Title = descriptor.Name;
		}
	}
	else
	{
		_windowSettings.Title = "JGEngine";
	}
#else
	// 패키지: Bin/JGPackage.json(쿠킹이 .jgproject 의 값을 옮겨 적음).
	HPackageDescriptor package;
	if (HPackageDescriptor::Load(HFileHelper::ProjectDirectory(), &package) == false)
	{
		return false;
	}

	*outGameModules = package.GameModules;
	_windowSettings = package.Window;
	if (_windowSettings.Title.Empty() == true)
	{
		_windowSettings.Title = package.Name;
	}
#endif

	// 명령줄이 설정보다 앞선다.
	const HCommandLineWindow commandLine = parseCommandLine();
	if (commandLine.bFullscreen == true)
	{
		_windowSettings.Mode = HGameWindowSettings::BorderlessFullscreenMode;
	}
	if (commandLine.bWindowed == true)
	{
		_windowSettings.Mode = HGameWindowSettings::WindowedMode;
	}
	if (commandLine.Width > 0 && commandLine.Height > 0)
	{
		_windowSettings.Width  = commandLine.Width;
		_windowSettings.Height = commandLine.Height;
	}
	if (commandLine.bNoVSync == true)
	{
		_windowSettings.bVSync = false;
	}

	return true;
}

bool HGameHostModule::createWindow()
{
	HJWindowArguments arguments;
	arguments.Title = _windowSettings.Title;
	arguments.bResizable = true;

	if (_windowSettings.Mode == HGameWindowSettings::BorderlessFullscreenMode)
	{
		// 크기는 무시되고 주 모니터 크기가 된다(GP-C1).
		arguments.Mode = EWindowMode::BorderlessFullscreen;
		arguments.Size = HVector2Int(_windowSettings.Width, _windowSettings.Height);
	}
	else
	{
		const int32 width  = (_windowSettings.Width > 0) ? _windowSettings.Width : 1920;
		const int32 height = (_windowSettings.Height > 0) ? _windowSettings.Height : 1080;
		HVector2Int client;
		arguments.Mode = EWindowMode::Windowed;
		arguments.Size = outerSizeForClient(HVector2Int(width, height), arguments.bResizable, &client);
	}

	_window = HPlatform::CreateJWindow(arguments);
	if (_window == nullptr)
	{
		return false;
	}

	// Graphics 는 시작할 때 이 창의 클라이언트 크기로 스왑체인을 만든다.
	GCoreSystem::GetGlobalValues().MainWindow = _window.GetRawPointer();
	return true;
}

void HGameHostModule::connectModules(const HList<PString>& gameModules)
{
	for (const char* moduleName : GAME_HOST_ENGINE_MODULES)
	{
		if (GModuleGlobalSystem::GetInstance().ConnectModule(moduleName) == false)
		{
			JG_LOG(GameHost, ELogLevel::Critical, "GameHost: fail connect %s", moduleName);
			continue;
		}

		_connectedEngineModules.push_back(moduleName);
	}

	// 게임 모듈의 StartupModule 이 엔트리 액터를 정하고 월드를 올린다(템플릿 {PROJECT_NAME}Module.cpp). 에디터 모듈은 연결하지 않는다.
	for (const PString& moduleName : gameModules)
	{
		if (GModuleGlobalSystem::GetInstance().ConnectModule(moduleName) == false)
		{
			JG_LOG(GameHost, ELogLevel::Error, "GameHost: fail connect game module %s", moduleName);
			continue;
		}

		_connectedGameModules.push_back(moduleName);
	}
}

void HGameHostModule::renderFrame()
{
	if (_viewport == nullptr || _window == nullptr)
	{
		return;
	}

	// 최소화(클라이언트 0×0) 동안은 그리지도 내지도 않고 잠깐 쉰다. 스왑체인 크기는 Graphics 가 맞춘다(GP-G1).
	const HVector2Int client = _window->GetClientSize();
	if (client.x <= 0 || client.y <= 0)
	{
		::Sleep(MINIMIZED_SLEEP_MS);
		return;
	}

	if (_framesSinceResize >= 0)
	{
		++_framesSinceResize;
		if (_framesSinceResize >= RESIZE_SETTLE_FRAMES)
		{
			_viewport->Resize((uint32)_pendingSize.x, (uint32)_pendingSize.y);
			_framesSinceResize = -1;
		}
	}

	PString problem;
	if (_viewport->Render(&problem) == false)
	{
		// 그릴 월드 · 카메라가 아직 없을 수 있다(엔진 단독, 로딩 중). 한 번만 알린다.
		if (_bLoggedRenderProblem == false && problem.Empty() == false)
		{
			JG_LOG(GameHost, ELogLevel::Info, "GameHost: nothing to draw yet (%s)", problem);
			_bLoggedRenderProblem = true;
		}
		return;
	}

	PSharedPtr<PJGGraphicsAPI> graphicsAPI = findGraphicsAPI();
	PSharedPtr<IRawTexture> displayTexture = _viewport->GetDisplayTexture();
	if (graphicsAPI != nullptr && displayTexture != nullptr)
	{
		graphicsAPI->SubmitFinalTexture(displayTexture);
	}

	// 게임 UI 글자 입력 중이면 OS IME 조합 창을 입력 커서 옆에 둔다.
	HRect caretRect;
	if (_viewport->WantsTextInput() == true && _viewport->GetTextInputCaretRect(&caretRect) == true)
	{
		HPlatform::SetImeCompositionRect(_window->GetHandle(), caretRect);
	}
}

void HGameHostModule::onResize(uint32 width, uint32 height)
{
	if (width == 0 || height == 0)
	{
		return;
	}

	_pendingSize = HVector2Int((int32)width, (int32)height);
	_framesSinceResize = 0;
}

void HGameHostModule::onInput(const HInputEvent& event)
{
	if (_viewport != nullptr)
	{
		// 창 클라이언트 = 뷰포트 픽셀. 어디로 갔는지(게임 UI · 월드)는 호스트가 쓰지 않는다.
		_viewport->HandleInput(event);
	}
}
