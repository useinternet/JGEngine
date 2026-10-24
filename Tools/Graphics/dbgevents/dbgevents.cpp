// Diagnostic: run a process under the debugger API, count debug events by type for N seconds,
// print the first unique OutputDebugString texts and first-chance exception codes, then WM_CLOSE.
// usage: dbgevents.exe <exe> <workdir> <closeAfterSeconds>
#include <windows.h>
#include <stdio.h>
#include <string>
#include <map>
#include <set>
#pragma comment(lib, "user32.lib")

static DWORD g_pid = 0;
static BOOL CALLBACK CloseProc(HWND hwnd, LPARAM)
{
	DWORD pid = 0;
	GetWindowThreadProcessId(hwnd, &pid);
	if (pid == g_pid && IsWindowVisible(hwnd) && GetWindow(hwnd, GW_OWNER) == nullptr)
	{
		PostMessageW(hwnd, WM_CLOSE, 0, 0);
		printf("WM_CLOSE posted\n");
	}
	return TRUE;
}

int wmain(int argc, wchar_t** argv)
{
	if (argc < 4) { printf("usage: dbgevents <exe> <workdir> <closeAfterSeconds>\n"); return 1; }
	std::wstring cmd = argv[1];
	STARTUPINFOW si = {}; si.cb = sizeof(si);
	PROCESS_INFORMATION pi = {};
	if (!CreateProcessW(nullptr, &cmd[0], nullptr, nullptr, FALSE, DEBUG_ONLY_THIS_PROCESS, nullptr, argv[2], &si, &pi)) { printf("CreateProcess failed %lu\n", GetLastError()); return 1; }
	g_pid = pi.dwProcessId;

	const ULONGLONG closeAfter = (ULONGLONG)_wtoi(argv[3]) * 1000ULL;
	const ULONGLONG start = GetTickCount64();
	ULONGLONG closedAt = 0;
	std::map<DWORD, unsigned long long> eventCounts;
	std::map<DWORD, unsigned long long> exceptionCounts;
	std::map<std::string, unsigned long long> strings;
	unsigned long long eventsAtClose = 0, totalEvents = 0;

	for (;;)
	{
		const ULONGLONG now = GetTickCount64();
		if (closedAt == 0 && now - start > closeAfter)
		{
			eventsAtClose = totalEvents;
			EnumWindows(CloseProc, 0);
			closedAt = now;
		}
		else if (closedAt != 0 && now - closedAt > 30000)
		{
			printf("no exit 30s after WM_CLOSE -> terminate\n");
			TerminateProcess(pi.hProcess, 4);
			closedAt = ~0ULL >> 1;
		}

		DEBUG_EVENT ev = {};
		if (!WaitForDebugEvent(&ev, 200))
		{
			continue;
		}
		++totalEvents;
		++eventCounts[ev.dwDebugEventCode];
		DWORD cont = DBG_CONTINUE;
		switch (ev.dwDebugEventCode)
		{
		case CREATE_PROCESS_DEBUG_EVENT:
			if (ev.u.CreateProcessInfo.hFile) CloseHandle(ev.u.CreateProcessInfo.hFile);
			break;
		case LOAD_DLL_DEBUG_EVENT:
			if (ev.u.LoadDll.hFile) CloseHandle(ev.u.LoadDll.hFile);
			break;
		case OUTPUT_DEBUG_STRING_EVENT:
		{
			const OUTPUT_DEBUG_STRING_INFO& info = ev.u.DebugString;
			std::string text;
			const SIZE_T len = info.nDebugStringLength;
			if (len > 0 && len < 4096)
			{
				if (info.fUnicode)
				{
					std::wstring w(len, L'\0');
					SIZE_T read = 0;
					ReadProcessMemory(pi.hProcess, info.lpDebugStringData, &w[0], len * sizeof(wchar_t), &read);
					text.assign(w.begin(), w.end());
				}
				else
				{
					text.resize(len);
					SIZE_T read = 0;
					ReadProcessMemory(pi.hProcess, info.lpDebugStringData, &text[0], len, &read);
				}
			}
			while (!text.empty() && (text.back() == '\0' || text.back() == '\n' || text.back() == '\r')) text.pop_back();
			if (text.size() > 200) text.resize(200);
			++strings[text];
			break;
		}
		case EXCEPTION_DEBUG_EVENT:
		{
			const DWORD code = ev.u.Exception.ExceptionRecord.ExceptionCode;
			++exceptionCounts[code];
			if (code == 0x406D1388 && ev.u.Exception.ExceptionRecord.NumberParameters >= 3)
			{
				// THREADNAME_INFO: [0]=0x1000, [1]=LPCSTR name, [2]=thread id
				char name[128] = {};
				SIZE_T read = 0;
				ReadProcessMemory(pi.hProcess, (LPCVOID)ev.u.Exception.ExceptionRecord.ExceptionInformation[1], name, sizeof(name) - 1, &read);
				std::string key = std::string("threadname: ") + name;
				++strings[key];
			}
			if (code == EXCEPTION_BREAKPOINT || code == 0x4000001F || code == 0x406D1388)
			{
				cont = DBG_CONTINUE;
			}
			else
			{
				cont = DBG_EXCEPTION_NOT_HANDLED;
				if (!ev.u.Exception.dwFirstChance)
				{
					printf("second-chance exception 0x%08lX -> terminate\n", code);
					TerminateProcess(pi.hProcess, 3);
				}
			}
			break;
		}
		case EXIT_PROCESS_DEBUG_EVENT:
		{
			printf("exit code %lu, elapsed %.1f s, events total %llu (before WM_CLOSE %llu)\n", ev.u.ExitProcess.dwExitCode, (GetTickCount64() - start) / 1000.0, totalEvents, eventsAtClose);
			const char* names[] = { "?", "EXCEPTION", "CREATE_THREAD", "CREATE_PROCESS", "EXIT_THREAD", "EXIT_PROCESS", "LOAD_DLL", "UNLOAD_DLL", "OUTPUT_DEBUG_STRING", "RIP" };
			for (auto& p : eventCounts) printf("  event %-20s %llu\n", p.first < 10 ? names[p.first] : "?", p.second);
			for (auto& p : exceptionCounts) printf("  exception 0x%08lX x %llu\n", p.first, p.second);
			int shown = 0;
			for (auto& p : strings) { if (shown++ >= 25) break; printf("  [x%llu] %s\n", p.second, p.first.c_str()); }
			printf("  unique debug strings: %zu\n", strings.size());
			ContinueDebugEvent(ev.dwProcessId, ev.dwThreadId, DBG_CONTINUE);
			return 0;
		}
		default:
			break;
		}
		ContinueDebugEvent(ev.dwProcessId, ev.dwThreadId, cont);
	}
}
