// crashwalk : 대상 프로세스를 디버기로 실행하고, 예외(AV)가 나면 심볼이 붙은 콜스택을 출력하는 최소 디버거.
// 사용: crashwalk.exe <exe> <workdir> <closeAfterSeconds>
//  - closeAfterSeconds 뒤에 대상의 최상위 창에 WM_CLOSE를 보내 정상 종료 경로를 밟게 한다.
#include <windows.h>
#include <dbghelp.h>
#include <psapi.h>
#include <stdio.h>
#include <string>
#include <vector>
#pragma comment(lib, "dbghelp.lib")
#pragma comment(lib, "user32.lib")
#pragma comment(lib, "psapi.lib")

static DWORD g_targetPid = 0;

// 주소 -> "모듈명+RVA" (심볼이 없어도 llvm-symbolizer로 추적할 수 있게)
static std::string DescribeAddress(HANDLE hProcess, DWORD64 addr)
{
	HMODULE mods[512];
	DWORD needed = 0;
	if (EnumProcessModulesEx(hProcess, mods, sizeof(mods), &needed, LIST_MODULES_ALL))
	{
		const DWORD count = needed / sizeof(HMODULE);
		for (DWORD i = 0; i < count; ++i)
		{
			MODULEINFO mi = {};
			if (!GetModuleInformation(hProcess, mods[i], &mi, sizeof(mi))) continue;
			const DWORD64 base = (DWORD64)mi.lpBaseOfDll;
			if (addr >= base && addr < base + mi.SizeOfImage)
			{
				char name[MAX_PATH] = {};
				GetModuleBaseNameA(hProcess, mods[i], name, MAX_PATH);
				char buf[MAX_PATH + 32];
				sprintf_s(buf, "%s+0x%llx", name, addr - base);
				return buf;
			}
		}
	}
	char buf[64];
	sprintf_s(buf, "0x%llx", addr);
	return buf;
}

static BOOL CALLBACK EnumWindowsProc(HWND hwnd, LPARAM)
{
	DWORD pid = 0;
	GetWindowThreadProcessId(hwnd, &pid);
	if (pid == g_targetPid && IsWindowVisible(hwnd) && GetWindow(hwnd, GW_OWNER) == nullptr)
	{
		PostMessageW(hwnd, WM_CLOSE, 0, 0);
		printf("WM_CLOSE posted to %p\n", hwnd);
	}
	return TRUE;
}

static void PrintStack(HANDLE hProcess, DWORD threadId)
{
	HANDLE hThread = OpenThread(THREAD_ALL_ACCESS, FALSE, threadId);
	if (hThread == nullptr) { printf("OpenThread failed %lu\n", GetLastError()); return; }

	CONTEXT ctx = {};
	ctx.ContextFlags = CONTEXT_FULL;
	if (!GetThreadContext(hThread, &ctx)) { printf("GetThreadContext failed %lu\n", GetLastError()); CloseHandle(hThread); return; }

	STACKFRAME64 sf = {};
	sf.AddrPC.Offset = ctx.Rip;    sf.AddrPC.Mode = AddrModeFlat;
	sf.AddrFrame.Offset = ctx.Rbp; sf.AddrFrame.Mode = AddrModeFlat;
	sf.AddrStack.Offset = ctx.Rsp; sf.AddrStack.Mode = AddrModeFlat;

	for (int i = 0; i < 80; ++i)
	{
		if (!StackWalk64(IMAGE_FILE_MACHINE_AMD64, hProcess, hThread, &sf, &ctx, nullptr, SymFunctionTableAccess64, SymGetModuleBase64, nullptr))
			break;
		if (sf.AddrPC.Offset == 0)
			break;

		char buf[sizeof(SYMBOL_INFO) + 1024] = {};
		SYMBOL_INFO* sym = (SYMBOL_INFO*)buf;
		sym->SizeOfStruct = sizeof(SYMBOL_INFO);
		sym->MaxNameLen = 1023;
		DWORD64 disp = 0;

		IMAGEHLP_MODULE64 mi = {};
		mi.SizeOfStruct = sizeof(mi);
		SymGetModuleInfo64(hProcess, sf.AddrPC.Offset, &mi);

		IMAGEHLP_LINE64 line = {};
		line.SizeOfStruct = sizeof(line);
		DWORD ldisp = 0;
		BOOL hasLine = SymGetLineFromAddr64(hProcess, sf.AddrPC.Offset, &ldisp, &line);

		const std::string where = DescribeAddress(hProcess, sf.AddrPC.Offset);
		if (SymFromAddr(hProcess, sf.AddrPC.Offset, &disp, sym))
			printf("  #%02d [%s] %s!%s+0x%llx  %s:%lu\n", i, where.c_str(), mi.ModuleName, sym->Name, disp, hasLine ? line.FileName : "", hasLine ? line.LineNumber : 0);
		else
			printf("  #%02d [%s] (no symbol)\n", i, where.c_str());
	}
	CloseHandle(hThread);
}

int wmain(int argc, wchar_t** argv)
{
	if (argc < 4) { printf("usage: crashwalk <exe> <workdir> <closeAfterSeconds>\n"); return 1; }

	std::wstring cmd = argv[1];
	STARTUPINFOW si = {}; si.cb = sizeof(si);
	PROCESS_INFORMATION pi = {};
	if (!CreateProcessW(nullptr, &cmd[0], nullptr, nullptr, FALSE, DEBUG_ONLY_THIS_PROCESS, nullptr, argv[2], &si, &pi))
	{
		printf("CreateProcess failed %lu\n", GetLastError());
		return 1;
	}
	g_targetPid = pi.dwProcessId;
	HANDLE hProcess = pi.hProcess;

	SymSetOptions(SYMOPT_UNDNAME | SYMOPT_DEFERRED_LOADS | SYMOPT_LOAD_LINES);
	SymInitializeW(hProcess, argv[2], FALSE);

	const ULONGLONG closeAfterMs = (ULONGLONG)_wtoi(argv[3]) * 1000ULL;
	const ULONGLONG start = GetTickCount64();
	bool bClosed = false;
	int exitCode = -1;

	for (;;)
	{
		DEBUG_EVENT ev = {};
		if (!WaitForDebugEvent(&ev, 200))
		{
			if (!bClosed && GetTickCount64() - start > closeAfterMs)
			{
				EnumWindows(EnumWindowsProc, 0);
				bClosed = true;
			}
			continue;
		}

		DWORD cont = DBG_CONTINUE;
		switch (ev.dwDebugEventCode)
		{
		case CREATE_PROCESS_DEBUG_EVENT:
			SymLoadModuleExW(hProcess, ev.u.CreateProcessInfo.hFile, nullptr, nullptr, (DWORD64)ev.u.CreateProcessInfo.lpBaseOfImage, 0, nullptr, 0);
			if (ev.u.CreateProcessInfo.hFile) CloseHandle(ev.u.CreateProcessInfo.hFile);
			break;
		case LOAD_DLL_DEBUG_EVENT:
		{
			wchar_t path[MAX_PATH] = {};
			if (ev.u.LoadDll.hFile)
			{
				GetFinalPathNameByHandleW(ev.u.LoadDll.hFile, path, MAX_PATH, 0);
				SymLoadModuleExW(hProcess, ev.u.LoadDll.hFile, path[0] ? path : nullptr, nullptr, (DWORD64)ev.u.LoadDll.lpBaseOfDll, 0, nullptr, 0);
				CloseHandle(ev.u.LoadDll.hFile);
			}
			break;
		}
		case UNLOAD_DLL_DEBUG_EVENT:
			printf("DLL unloaded at %p\n", ev.u.UnloadDll.lpBaseOfDll);
			SymUnloadModule64(hProcess, (DWORD64)ev.u.UnloadDll.lpBaseOfDll);
			break;
		case EXCEPTION_DEBUG_EVENT:
		{
			const EXCEPTION_RECORD& er = ev.u.Exception.ExceptionRecord;
			const DWORD code = er.ExceptionCode;
			if (code == EXCEPTION_BREAKPOINT || code == 0x4000001F || code == 0x40010006 || code == 0x4001000A || code == 0x406D1388)
			{
				cont = DBG_CONTINUE;
				break;
			}
			if (code == 0xE06D7363 /* C++ exception */ && ev.u.Exception.dwFirstChance)
			{
				cont = DBG_EXCEPTION_NOT_HANDLED;
				break;
			}

			printf("=== Exception 0x%08lX at %p (firstChance=%lu) thread %lu ===\n", code, er.ExceptionAddress, ev.u.Exception.dwFirstChance, ev.dwThreadId);
			if (code == EXCEPTION_ACCESS_VIOLATION && er.NumberParameters >= 2)
				printf("    %s address %p\n", er.ExceptionInformation[0] == 0 ? "read" : er.ExceptionInformation[0] == 1 ? "write" : "exec", (void*)er.ExceptionInformation[1]);
			// 예외 시점에 현재 매핑된 모듈 전체의 심볼을 다시 적재한다. (수동 적재가 빠진 모듈 대비)
			SymCleanup(hProcess);
			SymInitializeW(hProcess, argv[2], TRUE);
			PrintStack(hProcess, ev.dwThreadId);
			fflush(stdout);

			cont = DBG_EXCEPTION_NOT_HANDLED;
			if (!ev.u.Exception.dwFirstChance)
			{
				TerminateProcess(hProcess, 3);
			}
			break;
		}
		case EXIT_PROCESS_DEBUG_EVENT:
			exitCode = (int)ev.u.ExitProcess.dwExitCode;
			printf("Process exited with code %d (0x%08X)\n", exitCode, (unsigned)exitCode);
			ContinueDebugEvent(ev.dwProcessId, ev.dwThreadId, DBG_CONTINUE);
			SymCleanup(hProcess);
			CloseHandle(pi.hThread);
			CloseHandle(pi.hProcess);
			return exitCode == 0 ? 0 : 2;
		default:
			break;
		}
		ContinueDebugEvent(ev.dwProcessId, ev.dwThreadId, cont);
	}
}
