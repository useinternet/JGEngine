#pragma once
#include "Memory/Memory.h"
#include "String/String.h"
#include "Math/Vector.h"
#include "PlatformDefines.h"
#include "Misc/Delegate.h"
#include "Input/InputEvent.h"

struct HWindowCallBacks
{
#ifdef _PLATFORM_WINDOWS
	JG_DECLARE_MULTICAST_DELEGATE(HWndProc, HWND, uint32, WPARAM, LPARAM);
	HWndProc WndProc;
#endif

	JG_DECLARE_MULTICAST_DELEGATE(HOnResize, uint32, uint32);
	HOnResize OnResize;

	// 엔진 입력 이벤트(Input/InputEvent.h). 구독자가 있을 때만 플랫폼 창이 OS 입력 메시지를 바꿔 보내고 포인터를 잡는다.
	// 에디터(ImGui)는 구독하지 않고 WndProc 를 그대로 쓴다.
	JG_DECLARE_MULTICAST_DELEGATE(HOnInput, const HInputEvent&);
	HOnInput OnInput;
};

// 창 모드. 테두리 없는 전체 화면 = 테두리 · 제목 줄 없는 창을 모니터 크기로 띄운다(독점 전체 화면은 하지 않는다).
enum class EWindowMode
{
	Windowed,
	BorderlessFullscreen,
};

struct HJWindowArguments
{
	PString     Title;
	HVector2Int Size;                           // 창 모드의 창 바깥 크기(테두리 포함). 테두리 없는 전체 화면이면 무시하고 모니터 크기
	int32       IconID;
	EWindowMode Mode       = EWindowMode::Windowed;
	bool        bResizable = true;              // 창 모드에서 테두리를 끌어 크기를 바꿀 수 있나(끄면 최대화 단추도 없다)

	HJWindowArguments() : IconID(INDEX_NONE) {}
};

class PJWindow : public IMemoryObject
{
	friend class HJWindowCreater;
private:
	HJWindowArguments _args;

public:
	virtual ~PJWindow() = default;

	virtual HJWHandle GetHandle() const { return 0; }
	virtual HVector2Int GetClientSize() const { return HVector2Int(); }
public:

	bool IsValid() const;
	const HJWindowArguments& GetArguments() const;


protected:
	virtual bool ConstructWindow() { return false; }
};

class HJWindowCreater
{
	bool _bIsSuccess;
public:
	HJWindowCreater(PJWindow* pWindow, const HJWindowArguments& args);

	bool IsSuccess() const;
};
