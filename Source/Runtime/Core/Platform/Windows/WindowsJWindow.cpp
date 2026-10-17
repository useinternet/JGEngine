#include "PCH/PCH.h"
#include "WindowsJWindow.h"
#include "Thread/Scheduler.h"
#include "CoreSystem.h"

LRESULT CALLBACK WndProc(HWND hWnd, uint32_t msg, WPARAM wParam, LPARAM lParam);

PWindowsJWindow::~PWindowsJWindow()
{
	// 창이 이 객체보다 오래 남아도 창 프로시저가 지워진 객체를 보지 않게 끊는다.
	if (_handle != 0 && ::IsWindow(_handle))
	{
		::SetWindowLongPtrW(_handle, GWLP_USERDATA, 0);
	}
}

HJWHandle PWindowsJWindow::GetHandle() const
{
	return _handle;
}

HVector2Int PWindowsJWindow::GetClientSize() const
{
	RECT result;
	::GetClientRect(_handle, &result);

	return HVector2Int(result.right - result.left, result.bottom - result.top);
}

bool PWindowsJWindow::ConstructWindow()
{
	const HJWindowArguments& args = GetArguments();

	_handle = 0;

	HRawWString wTitle = args.Title.GetRawWString();

	HJModule hInstance = GetModuleHandle(NULL);

	WNDCLASSEXW wcex = {};
	wcex.cbSize = sizeof(WNDCLASSEX);
	wcex.style  = CS_HREDRAW | CS_VREDRAW;
	wcex.lpfnWndProc = WndProc;
	wcex.cbClsExtra = 0;
	wcex.cbWndExtra = 0;
	wcex.hInstance  = hInstance;

	if (args.IconID == -1) wcex.hIcon = LoadIcon(0, IDI_APPLICATION);
	else wcex.hIcon = LoadIcon(hInstance, MAKEINTRESOURCE(args.IconID));

	if (args.IconID == -1) wcex.hIconSm = LoadIcon(0, IDI_APPLICATION);
	else wcex.hIconSm = LoadIcon(hInstance, MAKEINTRESOURCE(args.IconID));

	wcex.hCursor       = LoadCursor(nullptr, IDC_ARROW);
	wcex.hbrBackground = (HBRUSH)(COLOR_WINDOW + 1);
	wcex.lpszMenuName  = 0;
	wcex.lpszClassName = wTitle.c_str();

	RegisterClassExW(&wcex);

	// 창 모드: 테두리 있는 창(크기 바꾸기를 끄면 끄는 테두리 · 최대화 단추 없음) 또는 주 모니터 크기의 테두리 없는 창.
	DWORD style  = WS_OVERLAPPEDWINDOW;
	int32 x      = CW_USEDEFAULT;
	int32 y      = CW_USEDEFAULT;
	int32 width  = args.Size.x;
	int32 height = args.Size.y;
	if (args.Mode == EWindowMode::BorderlessFullscreen)
	{
		MONITORINFO monitorInfo = {};
		monitorInfo.cbSize = sizeof(monitorInfo);
		::GetMonitorInfoW(::MonitorFromPoint(POINT{ 0, 0 }, MONITOR_DEFAULTTOPRIMARY), &monitorInfo);
		style  = WS_POPUP;
		x      = monitorInfo.rcMonitor.left;
		y      = monitorInfo.rcMonitor.top;
		width  = monitorInfo.rcMonitor.right - monitorInfo.rcMonitor.left;
		height = monitorInfo.rcMonitor.bottom - monitorInfo.rcMonitor.top;
	}
	else if (args.bResizable == false)
	{
		style &= ~(WS_THICKFRAME | WS_MAXIMIZEBOX);
	}

	_handle = CreateWindowW(
		wcex.lpszClassName, wcex.lpszClassName,
		style, x, y, width, height, nullptr, nullptr,
		hInstance, nullptr);
	if (_handle == 0)
	{
		return false;
	}

	// 창 프로시저가 입력 상태를 찾도록 창에 이 객체를 붙인다.
	::SetWindowLongPtrW(_handle, GWLP_USERDATA, (LONG_PTR)this);

	ShowWindow(_handle, SW_NORMAL);
	UpdateWindow(_handle);

	GScheduleGlobalSystem::GetInstance().ScheduleByFrame(EMainThreadExecutionOrder::UpdateWindow, PTaskDelegate::CreateRaw(this, &PWindowsJWindow::Update));

	return true;
}
void PWindowsJWindow::Update()
{
	MSG msg;
	while (::PeekMessage(&msg, NULL, 0, 0, PM_REMOVE))
	{
		if (msg.message != WM_QUIT)
		{
			TranslateMessage(&msg);
			DispatchMessage(&msg);
		}
		else
		{
			GCoreSystem::GetInstance().bIsRunning = false;
		}
	}
}

void PWindowsJWindow::dispatchInput(HWND hWnd, uint32 msg, WPARAM wParam, LPARAM lParam)
{
	if (msg == WM_MOUSELEAVE)
	{
		_bTrackingLeave = false;
	}

	EInputModifier modifiers = EInputModifier::None;
	if ((::GetKeyState(VK_SHIFT) & 0x8000) != 0)
	{
		modifiers |= EInputModifier::Shift;
	}
	if ((::GetKeyState(VK_CONTROL) & 0x8000) != 0)
	{
		modifiers |= EInputModifier::Ctrl;
	}
	if ((::GetKeyState(VK_MENU) & 0x8000) != 0)
	{
		modifiers |= EInputModifier::Alt;
	}

	const bool bWasAnyButtonDown = _input.IsAnyButtonDown();
	HInputEvent event;
	if (_input.Translate(msg, wParam, lParam, modifiers, event) == false)
	{
		return;
	}

	switch (event.Type)
	{
	case EInputEventType::PointerMove:
		// 창 밖으로 나가면 WM_MOUSELEAVE 가 오도록 한 번 등록한다(나간 뒤 다시 들어오면 또 등록).
		if (_bTrackingLeave == false)
		{
			TRACKMOUSEEVENT track = {};
			track.cbSize    = sizeof(track);
			track.dwFlags   = TME_LEAVE;
			track.hwndTrack = hWnd;
			_bTrackingLeave = ::TrackMouseEvent(&track) != FALSE;
		}
		break;
	case EInputEventType::PointerDown:
		// 버튼을 누르고 있는 동안 포인터를 잡는다 — 창 밖에서도 이동 · 뗌이 온다.
		if (bWasAnyButtonDown == false)
		{
			::SetCapture(hWnd);
		}
		break;
	case EInputEventType::PointerUp:
		if (_input.IsAnyButtonDown() == false && ::GetCapture() == hWnd)
		{
			::ReleaseCapture();
		}
		break;
	case EInputEventType::PointerWheel:
	{
		// 휠 메시지는 화면 좌표로 온다.
		POINT point = { (LONG)event.Position.x, (LONG)event.Position.y };
		::ScreenToClient(hWnd, &point);
		event.Position = HVector2((float32)point.x, (float32)point.y);
		break;
	}
	case EInputEventType::FocusLost:
		if (::GetCapture() == hWnd)
		{
			::ReleaseCapture();
		}
		break;
	default:
		break;
	}

	GCoreSystem::GetGlobalValues().WindowCallBacks->OnInput.BroadCast(event);
}

LRESULT WndProc(HWND hWnd, uint32_t msg, WPARAM wParam, LPARAM lParam)
{
	HCoreSystemGlobalValues& globalValues = GCoreSystem::GetGlobalValues();
	globalValues.WindowCallBacks->WndProc.BroadCast(hWnd, msg, wParam, lParam);

	// 엔진 입력 이벤트: 구독자(게임 호스트 등)가 있을 때만 바꾸고 포인터를 잡는다. 없으면(에디터) 아무것도 하지 않는다.
	PWindowsJWindow* window = (PWindowsJWindow*)::GetWindowLongPtrW(hWnd, GWLP_USERDATA);
	if (window != nullptr && globalValues.WindowCallBacks->OnInput.GetSize() > 0)
	{
		window->dispatchInput(hWnd, msg, wParam, lParam);
	}

	switch (msg)
	{
	case WM_SIZE:
		globalValues.WindowCallBacks->OnResize.BroadCast((uint32)LOWORD(lParam), (uint32)HIWORD(lParam));
		return 0;
	case WM_DPICHANGED:
	{
		// 창이 다른 배율의 모니터로 옮겨 갔다(DPI 인식 프로세스만 받는다). Windows 가 정해 준 크기 · 자리로 바꿔 보이는 크기를 유지한다.
		// 구독자(GUI 백엔드의 글꼴 · 스타일 배율)는 위 WndProc 브로드캐스트에서 이미 받았다. 크기가 바뀌면 WM_SIZE 가 따라온다.
		const RECT* suggested = (const RECT*)lParam;
		::SetWindowPos(hWnd, nullptr, suggested->left, suggested->top, suggested->right - suggested->left, suggested->bottom - suggested->top,
			SWP_NOZORDER | SWP_NOACTIVATE);
		return 0;
	}
	case WM_DESTROY:
		PostQuitMessage(0);
		return 0;
	}
	return DefWindowProc(hWnd, msg, wParam, lParam);
}
