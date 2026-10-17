#include "PCH/PCH.h"
#include "Platform.h"
#include "JWindow.h"
#include "Misc/Guid.h"
#include "Input/Key.h"
#include "FileIO/FileHelper.h"
#include "Math/Rect.h"

#ifdef _PLATFORM_WINDOWS
#include "Windows/WindowsJWindow.h"
#include <imm.h>
#pragma comment(lib, "imm32.lib")
#endif

void HPlatform::MemCopy(const void* src, void* dest, uint64 inSize)
{
#ifdef _PLATFORM_WINDOWS
	memcpy_s(dest, inSize, src, inSize);
#else
	JG_CHECK(false)
#endif
}

HJInstance HPlatform::LoadDll(const PString& dllPath)
{
#ifdef _PLATFORM_WINDOWS
	return ::LoadLibraryA(dllPath.GetCStr());
#else
	return 0;
#endif
}


void HPlatform::UnLoadDll(HJInstance instanceID)
{
	if (instanceID != nullptr)
	{
#ifdef _PLATFORM_WINDOWS
		::FreeLibrary(instanceID);
#else
#endif
	}
}

PString HPlatform::GetExecutableDirectory()
{
#ifdef _PLATFORM_WINDOWS
	// 긴 경로도 받도록 버퍼를 늘려 가며 읽는다. 길이가 버퍼와 같으면 잘린 것이다.
	std::wstring buffer(MAX_PATH, L'\0');
	DWORD length = 0;
	while (true)
	{
		length = ::GetModuleFileNameW(nullptr, buffer.data(), (DWORD)buffer.size());
		if (length == 0)
		{
			return PString();
		}

		if (length < buffer.size())
		{
			break;
		}

		buffer.resize(buffer.size() * 2);
	}
	buffer.resize(length);

	const std::wstring directory = fs::path(buffer).parent_path().lexically_normal().wstring();
	const int32 utf8Length = ::WideCharToMultiByte(CP_UTF8, 0, directory.c_str(), (int32)directory.size(), nullptr, 0, nullptr, nullptr);
	if (utf8Length <= 0)
	{
		return PString();
	}

	HRawString utf8(utf8Length, '\0');
	::WideCharToMultiByte(CP_UTF8, 0, directory.c_str(), (int32)directory.size(), utf8.data(), utf8Length, nullptr, nullptr);

	PString result = utf8.c_str();
	HFileHelper::NormalizePath(&result);
	if (result.EndWidth("/") == false)
	{
		result.Append("/");
	}

	return result;
#else
	return PString();
#endif
}

PSharedPtr<PJWindow> HPlatform::CreateJWindow(const HJWindowArguments& args)
{
#ifdef _PLATFORM_WINDOWS
	PSharedPtr<PJWindow> window = ::Allocate<PWindowsJWindow>();
	HJWindowCreater windowCreater(window.GetRawPointer(), args);
	if (windowCreater.IsSuccess())
	{
		return window;
	}

	return nullptr;
#else 
	return nullptr;
#endif
}

HGuid HPlatform::NewGuid()
{
	HGuid Guid;
#ifdef _PLATFORM_WINDOWS
	UUID newId;
	UuidCreate(&newId);

	Guid._data1 = newId.Data1;
	Guid._data2 = newId.Data2;
	Guid._data3 = newId.Data3;

	for (int32 i = 0; i < 8; ++i)
	{
		Guid._data4[i] = newId.Data4[i];
	}

#endif

	return Guid;
}

uint64 HPlatform::GuidHash(const HGuid& inGuid)
{
	if (inGuid.IsValid() == false)
	{
		return 0;
	}

	uint64 hash = 0;

#ifdef _PLATFORM_WINDOWS
	UUID uuid;

	uuid.Data1 = (unsigned long)inGuid._data1;
	uuid.Data2 = (unsigned long)inGuid._data2;
	uuid.Data3 = (unsigned long)inGuid._data3;
	for (int32 i = 0; i < 8; ++i)
	{
		uuid.Data4[i] = inGuid._data4[i];
	}

	RPC_STATUS rpcStatus;
	hash = ::UuidHash(&uuid, &rpcStatus);

#endif
	return hash;
}

int32 HPlatform::GetSystemKeyCode(EKeyCode keyCode)
{
	switch (keyCode)
	{
	case EKeyCode::A:	return 0x41;
	case EKeyCode::B:	return 0x42;
	case EKeyCode::C:	return 0x43;
	case EKeyCode::D:	return 0x44;
	case EKeyCode::E:	return 0x45;
	case EKeyCode::F:	return 0x46;
	case EKeyCode::G:	return 0x47;
	case EKeyCode::H:	return 0x48;
	case EKeyCode::I:	return 0x49;
	case EKeyCode::J:   return 0x4A;
	case EKeyCode::K:	return 0x4B;
	case EKeyCode::L:	return 0x4C;
	case EKeyCode::M:	return 0x4D;
	case EKeyCode::N:	return 0x4E;
	case EKeyCode::O:	return 0x4F;
	case EKeyCode::P:	return 0x50;
	case EKeyCode::Q:	return 0x51;
	case EKeyCode::R:	return 0x52;
	case EKeyCode::S:	return 0x53;
	case EKeyCode::T:	return 0x54;
	case EKeyCode::U:	return 0x55;
	case EKeyCode::V:	return 0x56;
	case EKeyCode::W:	return 0x57;
	case EKeyCode::X:	return 0x58;
	case EKeyCode::Y:	return 0x59;
	case EKeyCode::Z:	return 0x5A;
	case EKeyCode::Num0: return 0x30;
	case EKeyCode::Num1: return 0x31;
	case EKeyCode::Num2: return 0x32;
	case EKeyCode::Num3: return 0x33;
	case EKeyCode::Num4: return 0x34;
	case EKeyCode::Num5: return 0x35;
	case EKeyCode::Num6: return 0x36;
	case EKeyCode::Num7: return 0x37;
	case EKeyCode::Num8: return 0x38;
	case EKeyCode::Num9: return 0x39;
	case EKeyCode::NumPad0: return 0x60;
	case EKeyCode::NumPad1: return 0x61;
	case EKeyCode::NumPad2:	return 0x62;
	case EKeyCode::NumPad3:	return 0x63;
	case EKeyCode::NumPad4:	return 0x64;
	case EKeyCode::NumPad5:	return 0x65;
	case EKeyCode::NumPad6:	return 0x66;
	case EKeyCode::NumPad7:	return 0x67;
	case EKeyCode::NumPad8:	return 0x68;
	case EKeyCode::NumPad9:	return 0x69;

	case EKeyCode::NumPadAdd:		return VK_ADD;
	case EKeyCode::NumPadMultiply:	return VK_MULTIPLY;
	case EKeyCode::NumPadSubtract:	return VK_SUBTRACT;
	case EKeyCode::NumPadDemical:	return VK_DECIMAL;
	case EKeyCode::NumPadDivide:	return VK_DIVIDE;
	case EKeyCode::NumLock:			return VK_NUMLOCK;

	case EKeyCode::F1:  return 0x70;
	case EKeyCode::F2:  return 0x71;
	case EKeyCode::F3:  return 0x72;
	case EKeyCode::F4:  return 0x73;
	case EKeyCode::F5:  return 0x74;
	case EKeyCode::F6:  return 0x75;
	case EKeyCode::F7:  return 0x76;
	case EKeyCode::F8:  return 0x77;
	case EKeyCode::F9:  return 0x78;
	case EKeyCode::F10: return 0x79;
	case EKeyCode::F11: return 0x7A;
	case EKeyCode::F12: return 0x7B;

	case EKeyCode::Left:  return VK_LEFT;
	case EKeyCode::Right: return VK_RIGHT;
	case EKeyCode::Up:	  return VK_UP;
	case EKeyCode::Down:  return VK_DOWN;

	case EKeyCode::LeftBracket:		return VK_OEM_4;
	case EKeyCode::RightBracket:	return VK_OEM_6;
	case EKeyCode::Semicolon:		return VK_OEM_1;
	case EKeyCode::QuotationMark:	return VK_OEM_7;
	case EKeyCode::Comma:			return VK_OEM_COMMA;
	case EKeyCode::Period:			return VK_OEM_PERIOD;
	case EKeyCode::Slash:			return VK_OEM_2;

	case EKeyCode::Del:				return VK_DELETE;
	case EKeyCode::PgUp:			return VK_PRIOR;
	case EKeyCode::PgDown:			return VK_NEXT;
	case EKeyCode::End:				return VK_END;
	case EKeyCode::Home:			return VK_HOME;
	case EKeyCode::Insert:			return VK_INSERT;

	case EKeyCode::Esc:				return VK_ESCAPE;
	case EKeyCode::GraveAccent:		return VK_OEM_3;
	case EKeyCode::Tab:				return VK_TAB;
	case EKeyCode::CapsLock:		return VK_CAPITAL;

	case EKeyCode::Plus:			return VK_OEM_PLUS;
	case EKeyCode::Minus:			return VK_OEM_MINUS;
	case EKeyCode::BackSpace:		return VK_BACK;
	case EKeyCode::BackSlash:		return VK_OEM_5;
	case EKeyCode::Enter:			return VK_RETURN;

	case EKeyCode::Pause:			return VK_PAUSE;
	case EKeyCode::ScrollLock:		return VK_SCROLL;
	case EKeyCode::PrintScreen:		return VK_SNAPSHOT;

	case EKeyCode::Space:			return VK_SPACE;

	case EKeyCode::MouseLeftButton:	return VK_LBUTTON;
	case EKeyCode::MouseRightButton:return VK_RBUTTON;
	case EKeyCode::MouseMidButton:	return VK_MBUTTON;
	}
	
	return 0;
}

#ifdef _PLATFORM_WINDOWS
namespace
{
	// UTF-8 ↔ UTF-16 (클립보드 CF_UNICODETEXT)
	std::wstring toUtf16(const PString& text)
	{
		if (text.Empty())
		{
			return std::wstring();
		}
		const int32 length = ::MultiByteToWideChar(CP_UTF8, 0, text.GetCStr(), (int32)text.Size(), nullptr, 0);
		std::wstring result((size_t)HMath::Max(length, 0), L'\0');
		if (length > 0)
		{
			::MultiByteToWideChar(CP_UTF8, 0, text.GetCStr(), (int32)text.Size(), result.data(), length);
		}
		return result;
	}

	PString toUtf8(const wchar_t* text)
	{
		const int32 length = ::WideCharToMultiByte(CP_UTF8, 0, text, -1, nullptr, 0, nullptr, nullptr);
		if (length <= 1)
		{
			return PString();
		}
		std::string result((size_t)length, '\0');
		::WideCharToMultiByte(CP_UTF8, 0, text, -1, result.data(), length, nullptr, nullptr);
		result.resize((size_t)length - 1);   // 끝의 0
		return PString(result.c_str());
	}
}
#endif

PString HPlatform::GetClipboardText()
{
#ifdef _PLATFORM_WINDOWS
	if (::OpenClipboard(nullptr) == FALSE)
	{
		return PString();
	}

	PString result;
	HANDLE data = ::GetClipboardData(CF_UNICODETEXT);
	if (data != nullptr)
	{
		const wchar_t* text = (const wchar_t*)::GlobalLock(data);
		if (text != nullptr)
		{
			result = toUtf8(text);
			::GlobalUnlock(data);
		}
	}
	::CloseClipboard();
	return result;
#else
	return PString();
#endif
}

bool HPlatform::SetClipboardText(const PString& text)
{
#ifdef _PLATFORM_WINDOWS
	const std::wstring wide = toUtf16(text);
	HGLOBAL memory = ::GlobalAlloc(GMEM_MOVEABLE, (wide.size() + 1) * sizeof(wchar_t));
	if (memory == nullptr)
	{
		return false;
	}
	wchar_t* buffer = (wchar_t*)::GlobalLock(memory);
	if (buffer == nullptr)
	{
		::GlobalFree(memory);
		return false;
	}
	memcpy(buffer, wide.c_str(), (wide.size() + 1) * sizeof(wchar_t));
	::GlobalUnlock(memory);

	if (::OpenClipboard(nullptr) == FALSE)
	{
		::GlobalFree(memory);
		return false;
	}
	::EmptyClipboard();
	// 성공하면 메모리는 클립보드 것이 된다(해제하지 않는다).
	const bool bSet = ::SetClipboardData(CF_UNICODETEXT, memory) != nullptr;
	::CloseClipboard();
	if (bSet == false)
	{
		::GlobalFree(memory);
	}
	return bSet;
#else
	return false;
#endif
}

void HPlatform::SetImeCompositionRect(HJWHandle window, const HRect& caretRect)
{
#ifdef _PLATFORM_WINDOWS
	HIMC context = ::ImmGetContext(window);
	if (context == nullptr)
	{
		return;
	}

	// 조합 창은 커서 자리에, 후보 창은 커서 사각형을 피해(가리지 않게) 띄운다 — ImGui Win32 백엔드와 같은 방식.
	COMPOSITIONFORM composition = {};
	composition.dwStyle      = CFS_FORCE_POSITION;
	composition.ptCurrentPos = POINT{ (LONG)caretRect.left, (LONG)caretRect.top };
	::ImmSetCompositionWindow(context, &composition);

	CANDIDATEFORM candidate = {};
	candidate.dwIndex      = 0;
	candidate.dwStyle      = CFS_EXCLUDE;
	candidate.ptCurrentPos = POINT{ (LONG)caretRect.left, (LONG)caretRect.top };
	candidate.rcArea       = RECT{ (LONG)caretRect.left, (LONG)caretRect.top, (LONG)caretRect.right, (LONG)caretRect.bottom };
	::ImmSetCandidateWindow(context, &candidate);

	::ImmReleaseContext(window, context);
#endif
}

bool HPlatform::EnableDpiAwareness()
{
#ifdef _PLATFORM_WINDOWS
	// 이미 정해졌으면(매니페스트 · 앞선 호출) 실패한다 — 그래도 지금 상태를 보고 돌려준다.
	if (::SetProcessDpiAwarenessContext(DPI_AWARENESS_CONTEXT_PER_MONITOR_AWARE_V2) == FALSE)
	{
		::SetProcessDPIAware();
	}
	const DPI_AWARENESS awareness = ::GetAwarenessFromDpiAwarenessContext(::GetThreadDpiAwarenessContext());
	return awareness == DPI_AWARENESS_PER_MONITOR_AWARE;
#else
	return false;
#endif
}

float32 HPlatform::GetDpiScale(HJWHandle window)
{
#ifdef _PLATFORM_WINDOWS
	const UINT dpi = (window != nullptr) ? ::GetDpiForWindow(window) : ::GetDpiForSystem();
	if (dpi == 0)
	{
		return 1.0f;
	}
	return (float32)dpi / (float32)USER_DEFAULT_SCREEN_DPI;
#else
	return 1.0f;
#endif
}
