#include "PCH/PCH.h"
#include "AudioGlobalSystem.h"
#include "Platform/JWindow.h"
#include <algorithm>

namespace
{
	// 창이 뒤로 가거나 앞으로 올 때 전체 크기를 바꾸는 시간
	const float32 BACKGROUND_FADE_SECONDS = 0.25f;

	// 명령줄에 name(예: L"-nosound")이 있나
	bool hasCommandLineSwitch(const wchar_t* name)
	{
		int32 argc = 0;
		LPWSTR* argv = ::CommandLineToArgvW(::GetCommandLineW(), &argc);
		if (argv == nullptr)
		{
			return false;
		}

		bool bFound = false;
		for (int32 i = 1; i < argc; ++i)
		{
			if (_wcsicmp(argv[i], name) == 0)
			{
				bFound = true;
				break;
			}
		}

		::LocalFree(argv);
		return bFound;
	}
}

GAudioGlobalSystem::~GAudioGlobalSystem()
{
	Shutdown();
}

void GAudioGlobalSystem::Initialize()
{
	if (_engine.IsInitialized() == true || _bShutdown == true)
	{
		return;
	}

	// 창이 없는 실행(JGConsole · 자체 검사)과 -nosound 는 무음 장치로 돈다 — API 는 그대로, 소리만 나지 않는다.
	const bool bHeadless = (GCoreSystem::GetGlobalValues().MainWindow == nullptr);
	const bool bNoSound  = hasCommandLineSwitch(L"-nosound");

	HAudioEngineArguments args;
	args.Output = (bHeadless == true || bNoSound == true) ? EAudioOutput::NullDevice : EAudioOutput::Device;
	if (_engine.Initialize(args) == false)
	{
		JG_LOG(Audio, ELogLevel::Warning, "Audio: could not start the audio engine - sounds are disabled");
		return;
	}

	if (args.Output == EAudioOutput::NullDevice)
	{
		JG_LOG(Audio, ELogLevel::Info, "Audio: silent mode (%s) - sounds play on the null device", bNoSound ? "-nosound" : "no window");
	}
	else if (_engine.IsDeviceActive() == false)
	{
		JG_LOG(Audio, ELogLevel::Warning, "Audio: no audio output device - sounds play silently (%s)", _engine.GetDeviceDescription());
	}
	else
	{
		JG_LOG(Audio, ELogLevel::Info, "Audio: output device %s", _engine.GetDeviceDescription());
	}

	// 창이 뒤로 가는 것(WM_ACTIVATEAPP)을 본다. 입력 구독(OnInput)은 에디터 주 창의 입력 변환을 켜므로 쓰지 않는다.
	HWindowCallBacks* callBacks = GCoreSystem::GetGlobalValues().WindowCallBacks.get();
	if (callBacks != nullptr)
	{
		_wndProcHandle = callBacks->WndProc.AddRaw(this, &GAudioGlobalSystem::onWndProc);
	}
}

void GAudioGlobalSystem::Shutdown()
{
	if (_bShutdown == true)
	{
		return;
	}
	_bShutdown = true;

	// 엔진 종료(GCoreSystem::Destroy)에서는 창 콜백이 먼저 사라진다 — 그때는 뺄 구독도 없다
	if (GCoreSystem::HasInstance() == true)
	{
		HWindowCallBacks* callBacks = GCoreSystem::GetGlobalValues().WindowCallBacks.get();
		if (callBacks != nullptr && _wndProcHandle.IsValid() == true)
		{
			callBacks->WndProc.Remove(_wndProcHandle);
		}
	}

	_engine.Shutdown();
}

bool GAudioGlobalSystem::IsActive() const
{
	return _bShutdown == false && _engine.IsInitialized() == true;
}

HAudioEngine& GAudioGlobalSystem::GetEngine()
{
	return _engine;
}

void GAudioGlobalSystem::SetBackgroundMode(EAudioBackgroundMode mode, float32 lowerVolume)
{
	_backgroundMode   = mode;
	_backgroundVolume = (std::max)(lowerVolume, 0.0f);
	applyBackgroundGain(BACKGROUND_FADE_SECONDS);
}

void GAudioGlobalSystem::Update()
{
	if (_bShutdown == true)
	{
		return;
	}
	_engine.Update();
}

void GAudioGlobalSystem::Destroy()
{
	Shutdown();
}

void GAudioGlobalSystem::onWndProc(HWND hWnd, uint32 message, WPARAM wParam, LPARAM lParam)
{
	if (message != WM_ACTIVATEAPP)
	{
		return;
	}

	const bool bActive = (wParam != FALSE);
	if (bActive == _bAppActive)
	{
		return;
	}

	_bAppActive = bActive;
	applyBackgroundGain(BACKGROUND_FADE_SECONDS);
}

void GAudioGlobalSystem::applyBackgroundGain(float32 seconds)
{
	float32 gain = 1.0f;
	if (_bAppActive == false)
	{
		if (_backgroundMode == EAudioBackgroundMode::Lower)
		{
			gain = _backgroundVolume;
		}
		else if (_backgroundMode == EAudioBackgroundMode::Mute)
		{
			gain = 0.0f;
		}
	}
	_engine.FadeOutputGain(gain, seconds);
}
