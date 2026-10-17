#include "PCH/PCH.h"
#include "Core.h"
#include "Misc/Module.h"
#include <shellapi.h>

// 게임 패키지 실행 진입(DevOps GP-D2, 2026-10-02). 패키지 루트의 <Name>.exe(JGGameBootstrap 사본)가 Bin 을 DLL 경로로 잡고
// 이 DLL 의 JGGameLaunch_Main 을 같은 프로세스에서 부른다(사용자 결정 D-23 ①). JGLauncher 의 main 에 해당한다 —
// 패키지 모드로 GCoreSystem 을 만들고(엔진 루트 = 프로젝트 루트 = 패키지 루트, Content 마운트 · 모듈 목록은 Bin/JGPackage.json)
// 루프를 돌린 뒤 종료 코드를 돌려준다. 실행 모듈은 JGPackage.json 의 LaunchModule(기본 GameHost).
// 패키지는 JG_WITH_EDITOR 0 빌드(ConfirmGame · ReleaseGame)만이다 — 에디터 빌드의 이 DLL 은 실행을 거절한다(종료 코드 4).
// 로그는 <루트>/Saved/Logs/<exe 이름>.log(사용자 결정 D-23 ② — 패키지 폴더 안 Saved). 같은 패키지에서 여럿 띄우면 뒤에 뜬 것부터
// <exe 이름>_2.log, _3.log … 에 따로 쓴다(ER-024 — 아래 claimLogFile). 혼자 띄우면 이름 그대로다.
//
// 이 DLL 이 읽는 명령줄. 나머지(-host · -join · -windowed · -fullscreen · -res= …)는 각 모듈이 GetCommandLine 으로 읽는다.
//   -smoke=N         N 프레임 돈 뒤 종료 코드 0 으로 끝낸다(쿠킹 명령의 스모크 실행). 실패해도 메시지 상자를 띄우지 않는다
//   -log             콘솔 창을 열어 로그를 같이 보여 준다
//   -logfile=<경로>  로그 파일을 고른다. 상대 경로는 <루트>/Saved/Logs 기준(예: -logfile=Host.log). 쓰는 중이면 같은 규칙으로 _2 …
//   -launchmodule=X  실행 모듈을 바꾼다(검증용 — 예: 게임 모듈만 헤드리스로)
//
// 종료 코드: 0 정상, 2 패키지 서술자를 읽지 못함, 3 실행 모듈을 연결하지 못함(DLL 이 없거나 시작 실패), 4 에디터 빌드,
// 그 밖은 GCoreSystem::RequestExit 값.

namespace
{
	const int32 EXIT_CODE_NO_PACKAGE       = 2;
	const int32 EXIT_CODE_NO_LAUNCH_MODULE = 3;
	const int32 EXIT_CODE_EDITOR_BUILD     = 4;

	struct HGameLaunchOptions
	{
		int32        SmokeFrames = 0;
		bool         bConsole    = false;
		HRawString   LaunchModule;
		std::wstring LogFile;
	};

	HRawString toUtf8(const std::wstring& text)
	{
		if (text.empty() == true)
		{
			return HRawString();
		}

		const int32 length = ::WideCharToMultiByte(CP_UTF8, 0, text.c_str(), (int32)text.size(), nullptr, 0, nullptr, nullptr);
		if (length <= 0)
		{
			return HRawString();
		}

		HRawString result(length, '\0');
		::WideCharToMultiByte(CP_UTF8, 0, text.c_str(), (int32)text.size(), result.data(), length, nullptr, nullptr);
		return result;
	}

	HGameLaunchOptions parseOptions(const wchar_t* commandLine)
	{
		HGameLaunchOptions options;
		if (commandLine == nullptr)
		{
			return options;
		}

		int32 argc = 0;
		LPWSTR* argv = ::CommandLineToArgvW(commandLine, &argc);
		if (argv == nullptr)
		{
			return options;
		}

		const std::wstring smokeKey   = L"-smoke=";
		const std::wstring moduleKey  = L"-launchmodule=";
		const std::wstring logFileKey = L"-logfile=";
		for (int32 i = 1; i < argc; ++i)
		{
			const std::wstring argument = argv[i];
			if (argument.rfind(smokeKey, 0) == 0)
			{
				// Windows.h 의 max 매크로 때문에 std::max 를 쓰지 않는다.
				const int32 frames = _wtoi(argument.c_str() + smokeKey.size());
				options.SmokeFrames = (frames > 0) ? frames : 1;
			}
			else if (argument == L"-log")
			{
				options.bConsole = true;
			}
			else if (argument.rfind(moduleKey, 0) == 0)
			{
				options.LaunchModule = toUtf8(argument.substr(moduleKey.size()));
			}
			else if (argument.rfind(logFileKey, 0) == 0)
			{
				options.LogFile = argument.substr(logFileKey.size());
			}
		}

		::LocalFree(argv);
		return options;
	}

	// spdlog 콘솔 출력은 로그 시스템을 만들 때 표준 출력을 잡으므로 GCoreSystem::Create 전에 연다.
	void openConsole()
	{
		if (::AllocConsole() == FALSE)
		{
			return;
		}

		FILE* stream = nullptr;
		freopen_s(&stream, "CONOUT$", "w", stdout);
		freopen_s(&stream, "CONOUT$", "w", stderr);
	}

	// 로그 파일 고르기(ER-024, 2026-10-11). 같은 패키지에서 게임을 여럿 띄우면(한 PC 에서 방장 · 참가자 시험) 모두 같은 파일을 열어
	// 뒤에 뜬 쪽이 앞 기록을 지우고 줄이 섞였다(로그 시스템은 파일을 공유 모드로 연다). 그래서 쓸 파일에 바이트 범위 잠금을 하나 걸어
	// "이 이름은 이 실행이 쓴다"를 표시하고, 이미 잠겨 있으면 다음 이름(<이름>_2.log, <이름>_3.log …)을 쓴다. 혼자 띄우면 지금 이름
	// 그대로다(쿠킹 스모크 · 배포 실행기가 <Name>.log 를 읽는다). 잠금은 파일 끝 훨씬 뒤 범위라 로그 쓰기와 부딪치지 않고, 잡기는 OS 가
	// 한 번에 하나만 들어주며, 프로세스가 끝나면(죽어도) OS 가 푼다. 경로 표기가 달라도 같은 파일이면 같은 잠금이다.
	const int32  LOG_FILE_CANDIDATES = 32;
	const uint64 LOG_CLAIM_OFFSET    = 0x7FFFFFFF00000000ull;

	struct HLogFileClaim
	{
		fs::path DefaultPath;
		fs::path Path;
		HANDLE   Handle = INVALID_HANDLE_VALUE;   // 잠금을 쥔 핸들. 게임이 끝날 때까지 연다
	};

	// 기본 로그 파일 = <루트>/Saved/Logs/<exe 이름>.log. 쿠킹이 부트스트랩을 <Name>.exe 로 복사하므로 패키지 이름과 같다.
	// -logfile= 이 있으면 그 경로(상대 경로는 <루트>/Saved/Logs 기준).
	fs::path defaultLogFilePath(const std::wstring& packageDirectory, const HGameLaunchOptions& options)
	{
		const fs::path logDirectory = fs::path(packageDirectory) / L"Saved" / L"Logs";
		if (options.LogFile.empty() == false)
		{
			const fs::path requested(options.LogFile);
			if (requested.is_absolute() == true)
			{
				return requested;
			}

			return logDirectory / requested;
		}

		std::wstring exePath(MAX_PATH, L'\0');
		const DWORD length = ::GetModuleFileNameW(nullptr, exePath.data(), (DWORD)exePath.size());
		exePath.resize(length);

		std::wstring name = fs::path(exePath).stem().wstring();
		if (name.empty() == true)
		{
			name = L"Game";
		}

		return logDirectory / (name + L".log");
	}

	// 1 이면 기본 이름, 2 부터 <이름>_2<확장자> …
	fs::path logFileCandidate(const fs::path& defaultPath, int32 index)
	{
		if (index <= 1)
		{
			return defaultPath;
		}

		return defaultPath.parent_path() / (defaultPath.stem().wstring() + L"_" + std::to_wstring(index) + defaultPath.extension().wstring());
	}

	// 파일을 열고(없으면 빈 파일을 만든다 — 있는 내용은 건드리지 않는다) 잠금을 건다. 다른 실행이 쓰고 있으면 *outInUse = true.
	HANDLE tryClaimLogFile(const fs::path& path, bool* outInUse)
	{
		*outInUse = false;

		const HANDLE handle = ::CreateFileW(path.c_str(), GENERIC_READ, FILE_SHARE_READ | FILE_SHARE_WRITE | FILE_SHARE_DELETE, nullptr, OPEN_ALWAYS, FILE_ATTRIBUTE_NORMAL, nullptr);
		if (handle == INVALID_HANDLE_VALUE)
		{
			// 다른 프로그램이 파일을 혼자 쓰게 열어 둔 경우도 쓰는 중으로 본다.
			*outInUse = (::GetLastError() == ERROR_SHARING_VIOLATION);
			return INVALID_HANDLE_VALUE;
		}

		OVERLAPPED overlapped = {};
		overlapped.Offset     = (DWORD)(LOG_CLAIM_OFFSET & 0xFFFFFFFFull);
		overlapped.OffsetHigh = (DWORD)(LOG_CLAIM_OFFSET >> 32);
		if (::LockFileEx(handle, LOCKFILE_EXCLUSIVE_LOCK | LOCKFILE_FAIL_IMMEDIATELY, 0, 1, 0, &overlapped) == FALSE)
		{
			*outInUse = (::GetLastError() == ERROR_LOCK_VIOLATION);
			::CloseHandle(handle);
			return INVALID_HANDLE_VALUE;
		}

		return handle;
	}

	HLogFileClaim claimLogFile(const std::wstring& packageDirectory, const HGameLaunchOptions& options)
	{
		HLogFileClaim claim;
		claim.DefaultPath = defaultLogFilePath(packageDirectory, options);
		claim.Path        = claim.DefaultPath;

		std::error_code errorCode;
		fs::create_directories(claim.DefaultPath.parent_path(), errorCode);

		for (int32 index = 1; index <= LOG_FILE_CANDIDATES; ++index)
		{
			const fs::path candidate = logFileCandidate(claim.DefaultPath, index);
			bool bInUse = false;
			const HANDLE handle = tryClaimLogFile(candidate, &bInUse);
			if (handle != INVALID_HANDLE_VALUE)
			{
				claim.Path   = candidate;
				claim.Handle = handle;
				return claim;
			}

			// 열 수 없는 자리(권한 · 잘못된 경로)면 더 찾지 않고 예전처럼 기본 이름을 로그 시스템에 맡긴다.
			if (bInUse == false)
			{
				break;
			}
		}

		return claim;
	}

	void releaseLogFile(HLogFileClaim* claim)
	{
		if (claim->Handle != INVALID_HANDLE_VALUE)
		{
			// 잠금도 같이 풀린다.
			::CloseHandle(claim->Handle);
			claim->Handle = INVALID_HANDLE_VALUE;
		}
	}

	// 엔진 컨테이너가 GCoreSystem::Destroy 전에 정리되도록 진입 함수와 나눈다(JGLauncher · JGConsole 과 같은 규칙).
	int32 runGame(const HGameLaunchOptions& options, const HRawString& packageDirectory, const HLogFileClaim& logFile)
	{
		const HRawString logPath = toUtf8(logFile.Path.wstring());

		HCoreSystemArguments args;
		args.PackageDirectory = packageDirectory;
		args.LogFilePath      = logPath;
		args.LaunchModule     = options.LaunchModule;   // 비면 JGPackage.json 의 LaunchModule

		if (GCoreSystem::Create(args) == false)
		{
			// 패키지 서술자를 못 읽었다. 원인은 로그의 Critical 줄.
			GCoreSystem::Destroy();
			return EXIT_CODE_NO_PACKAGE;
		}

		// 어느 창의 기록인지 알 수 있게 프로세스 번호를 남긴다. 기본 이름을 다른 실행이 쓰고 있어 다른 이름을 골랐으면 그것도 남긴다.
		const uint32 processId = (uint32)::GetCurrentProcessId();
		if (logFile.Path != logFile.DefaultPath)
		{
			const HRawString defaultName = toUtf8(logFile.DefaultPath.filename().wstring());
			JG_LOG(GameLaunch, ELogLevel::Info, "Log file %s (process %u) - %s is in use by another run of this package", logPath.c_str(), processId, defaultName.c_str());
		}
		else
		{
			JG_LOG(GameLaunch, ELogLevel::Info, "Log file %s (process %u)", logPath.c_str(), processId);
		}

		// 실행 모듈이 없으면 게임이 없다. 빈 창 · 빈 루프로 0 을 돌려주지 않는다(원인은 로그의 "Fail Connect Module" 줄).
		const HRawString launchModule = options.LaunchModule.empty() ? GCoreSystem::GetGlobalValues().PackageLaunchModule : options.LaunchModule;
		if (GModuleGlobalSystem::GetInstance().FindModule(PName(launchModule.c_str())) == nullptr)
		{
			GCoreSystem::Destroy();
			return EXIT_CODE_NO_LAUNCH_MODULE;
		}

		int32 frameCount = 0;
		while (GCoreSystem::Update())
		{
			if (options.SmokeFrames > 0)
			{
				++frameCount;
				if (frameCount >= options.SmokeFrames)
				{
					GCoreSystem::RequestExit(0);
				}
			}
		}

		// Destroy 뒤에는 GCoreSystem 이 없으므로 먼저 읽는다.
		const int32 exitCode = GCoreSystem::GetExitCode();
		GCoreSystem::Destroy();
		return exitCode;
	}
}

extern "C" __declspec(dllexport) int JGGameLaunch_Main(const wchar_t* packageDirectory, const wchar_t* commandLine)
{
	if (packageDirectory == nullptr)
	{
		return EXIT_CODE_NO_PACKAGE;
	}

	const HGameLaunchOptions options = parseOptions(commandLine);

#if JG_WITH_EDITOR
	// 에디터 빌드(DevelopEngine · DevelopGame)는 엔진 · 프로젝트 폴더 경로로 돈다. 패키지는 ConfirmGame · ReleaseGame 으로 쿠킹한다.
	if (options.SmokeFrames == 0)
	{
		::MessageBoxW(nullptr, L"This is an editor build. Cook the game with -config=ConfirmGame or ReleaseGame.", L"Game", MB_OK | MB_ICONERROR);
	}

	return EXIT_CODE_EDITOR_BUILD;
#else
	if (options.bConsole == true)
	{
		openConsole();
	}

	const std::wstring root = packageDirectory;
	HLogFileClaim logFile = claimLogFile(root, options);
	const int32 exitCode = runGame(options, toUtf8(root), logFile);
	releaseLogFile(&logFile);

	if (options.SmokeFrames == 0 && (exitCode == EXIT_CODE_NO_PACKAGE || exitCode == EXIT_CODE_NO_LAUNCH_MODULE))
	{
		const std::wstring reason = (exitCode == EXIT_CODE_NO_PACKAGE) ? L"Failed to read Bin\\JGPackage.json in " : L"Failed to start the game module in ";
		const std::wstring message = reason + root + L"\nThe game files are incomplete. Reinstall the game. Details: Saved\\Logs";
		::MessageBoxW(nullptr, message.c_str(), L"Game", MB_OK | MB_ICONERROR);
	}

	return exitCode;
#endif
}
