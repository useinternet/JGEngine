#pragma once
#include "CoreDefines.h"
#include "PlatformDefines.h"
#include "String/String.h"


enum class EKeyCode;

template<class ReturnType, class ...Args>
class HPlatformFunction
{
	friend class HPlatform;
private:
	ReturnType(*func)(Args...);
public:
	HPlatformFunction() = default;
	HPlatformFunction(ReturnType(*inFunc)(Args...)) : func(inFunc) {}
	ReturnType operator()(Args... args)
	{
		return func(args...);
	}
	bool IsVaild() const {
		return func != nullptr;
	}
};

class PJWindow;
class HGuid;
class HRect;
struct HJWindowArguments;
class HPlatform
{
public:
	template<class T, class ... Args>
	static T* Allocate(Args ... args)
	{
		return new T(args...);
	}

	template<class T>
	static void Deallocate(T*& inPtr)
	{
		if (inPtr == nullptr)
		{
			return;
		}

		delete inPtr;
		inPtr = nullptr;
	}

	static void MemCopy(const void* src, void* dest, uint64 inSize);

	// -- Dll --
	static HJInstance LoadDll(const PString& dllPath);
	static void UnLoadDll(HJInstance instance);

	// 이 프로세스 실행 파일(exe)이 있는 폴더. 절대경로 · '/' 구분자 · 끝에 '/', UTF-8. 작업 폴더와 무관하다.
	// DLL 안에서 불러도 프로세스 exe 기준이다(패키지에서는 루트의 <Name>.exe). 실패하면 빈 문자열.
	static PString GetExecutableDirectory();

	template<class Return, class ... Args>
	static HPlatformFunction<Return, Args...> LoadFuncInDll(HJInstance instance, const PString& funcName)
	{
		HPlatformFunction<Return, Args...> result;

		if (instance != nullptr)
		{
			result.func = (Return(*)(Args...))::GetProcAddress(instance, funcName.GetCStr());
		}

		return result;
	}
	//


	// -- Windows --
	static PSharedPtr<PJWindow> CreateJWindow(const HJWindowArguments& args);
	static HGuid  NewGuid();
	static uint64 GuidHash(const HGuid& hash);

	static int32    GetSystemKeyCode(EKeyCode inKeyCode);

	// -- 클립보드 · IME (GP-C1) --
	// 클립보드의 글(UTF-8). 글이 없거나 클립보드를 열지 못하면 빈 문자열.
	static PString GetClipboardText();
	// 클립보드에 글(UTF-8)을 넣는다. 실패하면 false.
	static bool    SetClipboardText(const PString& text);
	// 글자 입력 중 OS IME 조합 창 · 후보 창을 window 클라이언트 픽셀의 caretRect(입력 커서 사각형) 옆에 띄운다(후보 창은 커서를 가리지 않게).
	static void    SetImeCompositionRect(HJWHandle window, const HRect& caretRect);

	// -- 화면 배율 (D-22 ②) --
	// 프로세스를 모니터별 DPI 인식(Per-Monitor V2)으로 바꾼다. 창 좌표 · 크기가 실제 픽셀이 되고 Windows 가 창을 늘려 흐리게 그리지 않는다.
	// 창을 하나라도 만들기 전에 부른다(프로그램 시작 코드). 안 되면(옛 Windows) 시스템 DPI 인식으로 낮춘다. 모니터별 인식이 되면 true.
	static bool    EnableDpiAwareness();
	// 화면 배율(1.0 = 96 DPI, 125% = 1.25). window 가 있으면 그 창이 있는 모니터 기준, 없으면 시스템(주 모니터) 기준.
	// 프로세스가 DPI 인식이 아니면 Windows 가 늘려 주므로 늘 1.0 이다.
	static float32 GetDpiScale(HJWHandle window = nullptr);
};