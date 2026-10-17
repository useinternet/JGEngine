// 게임 패키지 부트스트랩. 패키지 루트의 <Name>.exe 가 이 프로그램의 사본이다(쿠킹 명령이 이름을 바꿔 복사 — DevOps GP-D2, 2026-10-02).
// 엔진과 무관한 단독 실행 파일이다(module.json "Standalone": 엔진 PCH · Core 없음, CRT 정적 링크). 하는 일:
//   1) exe 폴더 = 패키지 루트
//   2) <루트>/Bin 을 DLL 검색 경로에 넣는다 — 엔진 · 게임 DLL, 3rd-party, 앱 로컬 CRT 가 모두 Bin 에 있다
//   3) Bin/JGGameLaunch.dll 의 JGGameLaunch_Main(루트, 명령줄)을 같은 프로세스에서 부르고 그 종료 코드를 그대로 돌려준다(사용자 결정 D-23 ①)
// 루트 exe 가 엔진을 직접 링크하지 않는 까닭: 엔진은 /MD(CRT DLL)라 그러면 CRT DLL 을 루트에 둬야 한다. 루트에는 Bin · Content · exe 만 둔다.

#include <windows.h>
#include <string>

namespace
{
	using HGameLaunchMain = int (*)(const wchar_t* packageDirectory, const wchar_t* commandLine);

	std::wstring executablePath()
	{
		std::wstring buffer(MAX_PATH, L'\0');
		while (true)
		{
			const DWORD length = ::GetModuleFileNameW(nullptr, buffer.data(), (DWORD)buffer.size());
			if (length == 0)
			{
				return std::wstring();
			}

			if (length < buffer.size())
			{
				buffer.resize(length);
				return buffer;
			}

			buffer.resize(buffer.size() * 2);
		}
	}

	// 쿠킹 명령의 스모크 실행(-smoke=N)은 사람이 없다 — 메시지 상자 없이 종료 코드로만 알린다.
	bool isUnattended(const wchar_t* commandLine)
	{
		return commandLine != nullptr && std::wstring(commandLine).find(L"-smoke=") != std::wstring::npos;
	}

	int fail(bool bUnattended, const std::wstring& title, const std::wstring& message)
	{
		if (bUnattended == false)
		{
			::MessageBoxW(nullptr, message.c_str(), title.c_str(), MB_OK | MB_ICONERROR);
		}

		return 1;
	}
}

int WINAPI wWinMain(_In_ HINSTANCE, _In_opt_ HINSTANCE, _In_ PWSTR, _In_ int)
{
	// 창을 만들기 전에 정한다. 게임 화면을 실제 픽셀로 그린다(Windows 가 늘려 흐리게 하지 않음). 실패해도(옛 Windows) 그대로 간다.
	::SetProcessDpiAwarenessContext(DPI_AWARENESS_CONTEXT_PER_MONITOR_AWARE_V2);

	const wchar_t* commandLine = ::GetCommandLineW();
	const bool bUnattended = isUnattended(commandLine);

	const std::wstring exePath = executablePath();
	const size_t separator = exePath.find_last_of(L"\\/");
	if (exePath.empty() == true || separator == std::wstring::npos)
	{
		return fail(bUnattended, L"Game", L"Failed to find the game folder.");
	}

	const std::wstring packageDirectory = exePath.substr(0, separator + 1);
	std::wstring title = exePath.substr(separator + 1);
	const size_t extension = title.find_last_of(L'.');
	if (extension != std::wstring::npos)
	{
		title.resize(extension);
	}

	const std::wstring binDirectory = packageDirectory + L"Bin";
	if (::SetDllDirectoryW(binDirectory.c_str()) == FALSE)
	{
		return fail(bUnattended, title, L"Failed to use the Bin folder: " + binDirectory);
	}

	const std::wstring launchPath = binDirectory + L"\\JGGameLaunch.dll";
	HMODULE launchModule = ::LoadLibraryW(launchPath.c_str());
	if (launchModule == nullptr)
	{
		const DWORD error = ::GetLastError();
		return fail(bUnattended, title, L"Failed to load Bin\\JGGameLaunch.dll (error " + std::to_wstring(error) + L").\nThe game files are incomplete. Reinstall the game.");
	}

	HGameLaunchMain launchMain = reinterpret_cast<HGameLaunchMain>(::GetProcAddress(launchModule, "JGGameLaunch_Main"));
	if (launchMain == nullptr)
	{
		return fail(bUnattended, title, L"Bin\\JGGameLaunch.dll has no JGGameLaunch_Main.\nThe game files are incomplete. Reinstall the game.");
	}

	// 게임이 끝날 때까지 돌아오지 않는다. DLL 은 프로세스 종료가 내린다(엔진 DLL 의 해제 순서를 여기서 바꾸지 않는다).
	return launchMain(packageDirectory.c_str(), commandLine);
}
