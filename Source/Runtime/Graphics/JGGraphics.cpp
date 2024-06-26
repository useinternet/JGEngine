#include "PCH/PCH.h"
#include "JGGraphics.h"
#include "JGGraphicsAPI.h"
#include "Classes/ShaderLibrary.h"

#ifdef _PLATFORM_WINDOWS
#include "Platform/Windows/WindowsJWindow.h"
#endif

#ifdef _DIRECTX12
#include "DirectX12/DirectX12API.h"

#endif

JG_MODULE_IMPL(HJGGraphicsModule, GRAPHICS_C_API)

JGType HJGGraphicsModule::GetModuleType() const
{
	return JGTYPE(HJGGraphicsModule);
}

void HJGGraphicsModule::StartupModule()
{
	// 나중에 Engine Setting 값으로 뺄 예정
	// 일단 DirectX12 API 원픽
#ifdef _DIRECTX12
	_graphicsAPI = Allocate<PDirectX12API>();

	//_graphicsAPI
#else
	JG_ASSERT("not supported graphics api");
#endif

	HCoreSystemGlobalValues& globalValues = GCoreSystem::GetGlobalValues();
	HVector2Int clientSize = globalValues.MainWindow->GetClientSize();

	HJGGraphicsArguments arguments;
	arguments.Handle = (uint64)globalValues.MainWindow->GetHandle();
	arguments.Width  = clientSize.x;
	arguments.Height = clientSize.y;
	arguments.ClearColor = HLinearColor(0, 0, 0, 0);
	arguments.BufferCount = 3;
	
	_graphicsAPI->Initialize(arguments);
	GCoreSystem::GetInstance().RegisterSystemInstance<GShaderLibrary>();

	GScheduleGlobalSystem::GetInstance().ScheduleByFrame(EMainThreadExecutionOrder::GraphicsBegin, PTaskDelegate::CreateRaw(this, &HJGGraphicsModule::BeginFrame));
	GScheduleGlobalSystem::GetInstance().ScheduleByFrame(EMainThreadExecutionOrder::GraphicsEnd, PTaskDelegate::CreateRaw(this, &HJGGraphicsModule::EndFrame));

	JG_LOG(Graphics, ELogLevel::Trace, "Startup Graphics Module...");
}

void HJGGraphicsModule::ShutdownModule()
{
	GCoreSystem::GetInstance().UnRegisterSystemInstance<GShaderLibrary>();
	_graphicsAPI->Destroy();
	_graphicsAPI = nullptr;

	JG_LOG(Graphics, ELogLevel::Trace, "Shutdown Graphics Module...");
}

void HJGGraphicsModule::BeginFrame()
{
	if (_graphicsAPI == nullptr)
	{
		return;
	}
	
	_graphicsAPI->BeginFrame();
}

void HJGGraphicsModule::EndFrame()
{
	if (_graphicsAPI == nullptr)
	{
		return;
	}

	_graphicsAPI->EndFrame();
}

PSharedPtr<PJGGraphicsAPI> HJGGraphicsModule::GetGraphicsAPI() const
{
	return _graphicsAPI;
}


PJGGraphicsAPI& GetGraphicsAPI()
{
	static PJGGraphicsAPI* API = nullptr;
	if (API == nullptr)
	{
		API = GModuleGlobalSystem::GetInstance().FindModule<HJGGraphicsModule>()->GetGraphicsAPI().GetRawPointer();
	}

	return *API;
}