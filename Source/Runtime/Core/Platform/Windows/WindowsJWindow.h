#pragma once
#include "Platform/JWindow.h"
#include "Platform/Windows/WindowsInput.h"
#include "Memory/Memory.h"
#include <functional>


class PTaskArguments;
class PWindowsJWindow : public PJWindow
{
	friend LRESULT WndProc(HWND hWnd, uint32_t msg, WPARAM wParam, LPARAM lParam);

private:
	HJWHandle _handle;
	HWindowsInputTranslator _input;            // 엔진 입력 이벤트(HWindowCallBacks::OnInput) 변환 상태
	bool                    _bTrackingLeave = false;

public:
	virtual ~PWindowsJWindow();
	virtual HJWHandle GetHandle() const override;
	virtual HVector2Int GetClientSize() const override;
protected:
	virtual bool ConstructWindow() override;

private:
	void Update();
	// 입력 메시지를 엔진 이벤트로 바꿔 OnInput 으로 보낸다. 버튼을 누르는 동안 포인터를 잡고, 나감을 추적한다.
	void dispatchInput(HWND hWnd, uint32 msg, WPARAM wParam, LPARAM lParam);
};


