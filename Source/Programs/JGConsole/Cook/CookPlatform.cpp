#include "PCH/PCH.h"
#include "Cook/CookPlatform.h"
#include <fstream>

namespace fsys = std::filesystem;

namespace
{
	std::wstring toWide(const PString& text)
	{
		const HRawString& raw = text.GetRawString();
		if (raw.empty() == true)
		{
			return std::wstring();
		}

		const int32 length = ::MultiByteToWideChar(CP_UTF8, 0, raw.c_str(), (int32)raw.size(), nullptr, 0);
		std::wstring result(length, L'\0');
		::MultiByteToWideChar(CP_UTF8, 0, raw.c_str(), (int32)raw.size(), result.data(), length);
		return result;
	}

	PString fromWide(const std::wstring& text)
	{
		if (text.empty() == true)
		{
			return PString();
		}

		const int32 length = ::WideCharToMultiByte(CP_UTF8, 0, text.c_str(), (int32)text.size(), nullptr, 0, nullptr, nullptr);
		HRawString result(length, '\0');
		::WideCharToMultiByte(CP_UTF8, 0, text.c_str(), (int32)text.size(), result.data(), length, nullptr, nullptr);
		return PString(result.c_str());
	}

	HRawString toLowerAscii(const HRawString& text)
	{
		HRawString result = text;
		for (char& c : result)
		{
			if (c >= 'A' && c <= 'Z')
			{
				c = (char)(c - 'A' + 'a');
			}
		}

		return result;
	}

	// 실행 결과의 첫 줄(앞뒤 공백 · 줄바꿈 제거). vswhere 출력 읽기용.
	PString runAndReadFirstLine(const PString& commandLine)
	{
		const fsys::path outputPath = fsys::temp_directory_path() / (L"jgcook_" + std::to_wstring(::GetCurrentProcessId()) + L".txt");
		const HCookPlatform::HProcessResult result = HCookPlatform::RunProcess(commandLine, PString(), HCookPlatform::FromPath(outputPath), 30000);

		PString firstLine;
		if (result.bStarted == true && result.bTimedOut == false && result.ExitCode == 0)
		{
			std::ifstream stream(outputPath);
			HRawString line;
			while (std::getline(stream, line))
			{
				while (line.empty() == false && (line.back() == '\r' || line.back() == ' ' || line.back() == '\t'))
				{
					line.pop_back();
				}

				if (line.empty() == false)
				{
					firstLine = line.c_str();
					break;
				}
			}
		}

		std::error_code errorCode;
		fsys::remove(outputPath, errorCode);
		return firstLine;
	}

	PString vswherePath()
	{
		wchar_t programFiles[MAX_PATH] = {};
		const DWORD length = ::GetEnvironmentVariableW(L"ProgramFiles(x86)", programFiles, MAX_PATH);
		if (length == 0 || length >= MAX_PATH)
		{
			return PString();
		}

		const fsys::path path = fsys::path(programFiles) / L"Microsoft Visual Studio" / L"Installer" / L"vswhere.exe";
		std::error_code errorCode;
		if (fsys::exists(path, errorCode) == false)
		{
			return PString();
		}

		return HCookPlatform::FromPath(path);
	}

	// "14.42.34433" → 숫자 묶음. 숫자 폴더가 아니면(v143 등) 빈 값.
	std::vector<int32> parseVersion(const std::wstring& name)
	{
		std::vector<int32> parts;
		int32 value = -1;
		for (wchar_t c : name)
		{
			if (c >= L'0' && c <= L'9')
			{
				value = (value < 0 ? 0 : value) * 10 + (int32)(c - L'0');
			}
			else if (c == L'.')
			{
				if (value < 0)
				{
					return std::vector<int32>();
				}

				parts.push_back(value);
				value = -1;
			}
			else
			{
				return std::vector<int32>();
			}
		}

		if (value < 0)
		{
			return std::vector<int32>();
		}

		parts.push_back(value);
		return parts;
	}

	// RVA → 파일 오프셋. 범위를 벗어나면 0.
	uint64 rvaToOffset(const IMAGE_SECTION_HEADER* sections, uint32 sectionCount, uint32 rva, uint64 fileSize)
	{
		for (uint32 i = 0; i < sectionCount; ++i)
		{
			const IMAGE_SECTION_HEADER& section = sections[i];
			const uint32 size = (section.Misc.VirtualSize > section.SizeOfRawData) ? section.Misc.VirtualSize : section.SizeOfRawData;
			if (rva >= section.VirtualAddress && rva < section.VirtualAddress + size)
			{
				const uint64 offset = (uint64)rva - section.VirtualAddress + section.PointerToRawData;
				return (offset < fileSize) ? offset : 0;
			}
		}

		return 0;
	}

	PString readCString(const std::vector<uint8>& data, uint64 offset)
	{
		HRawString text;
		for (uint64 i = offset; i < data.size() && data[i] != 0; ++i)
		{
			text.push_back((char)data[i]);
		}

		return PString(toLowerAscii(text).c_str());
	}
}

namespace HCookPlatform
{
	fsys::path ToPath(const PString& path)
	{
		return fsys::path(toWide(path));
	}

	PString FromPath(const fsys::path& path)
	{
		return fromWide(path.wstring());
	}

	PString NormalizedAbsolute(const PString& path)
	{
		std::error_code errorCode;
		fsys::path absolutePath = fsys::absolute(ToPath(path), errorCode).lexically_normal();

		PString result = FromPath(absolutePath);
		result.ReplaceAll("\\", "/");
		while (result.Length() > 3 && result.EndWidth("/") == true)
		{
			PString trimmed;
			result.SubString(&trimmed, 0, result.Length() - 1);
			result = trimmed;
		}

		return result;
	}

	bool IsSameOrAncestor(const PString& a, const PString& b)
	{
		const HRawString lowerA = toLowerAscii(NormalizedAbsolute(a).GetRawString());
		const HRawString lowerB = toLowerAscii(NormalizedAbsolute(b).GetRawString());
		if (lowerA == lowerB)
		{
			return true;
		}

		const HRawString prefix = (lowerA.empty() == false && lowerA.back() == '/') ? lowerA : lowerA + "/";
		return lowerB.rfind(prefix, 0) == 0;
	}

	HProcessResult RunProcess(const PString& commandLine, const PString& workingDirectory, const PString& logPath, uint32 timeoutMs)
	{
		HProcessResult result;

		SECURITY_ATTRIBUTES security = {};
		security.nLength = sizeof(security);
		security.bInheritHandle = TRUE;

		const std::wstring wideLogPath = toWide(logPath);
		HANDLE logHandle = ::CreateFileW(wideLogPath.c_str(), GENERIC_WRITE, FILE_SHARE_READ, &security, CREATE_ALWAYS, FILE_ATTRIBUTE_NORMAL, nullptr);
		if (logHandle == INVALID_HANDLE_VALUE)
		{
			return result;
		}

		// 표준 입력은 NUL — 자식이 입력을 기다리며 멈추지 않게(배치의 pause 등).
		HANDLE inputHandle = ::CreateFileW(L"NUL", GENERIC_READ, FILE_SHARE_READ | FILE_SHARE_WRITE, &security, OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL, nullptr);

		STARTUPINFOW startup = {};
		startup.cb = sizeof(startup);
		startup.dwFlags = STARTF_USESTDHANDLES;
		startup.hStdInput  = inputHandle;
		startup.hStdOutput = logHandle;
		startup.hStdError  = logHandle;

		PROCESS_INFORMATION process = {};
		std::wstring wideCommandLine = toWide(commandLine);
		const std::wstring wideWorkingDirectory = toWide(workingDirectory);

		const BOOL bCreated = ::CreateProcessW(nullptr, wideCommandLine.data(), nullptr, nullptr, TRUE, CREATE_NO_WINDOW, nullptr,
			wideWorkingDirectory.empty() ? nullptr : wideWorkingDirectory.c_str(), &startup, &process);

		::CloseHandle(logHandle);
		if (inputHandle != INVALID_HANDLE_VALUE)
		{
			::CloseHandle(inputHandle);
		}

		if (bCreated == FALSE)
		{
			return result;
		}

		result.bStarted = true;
		const DWORD waitResult = ::WaitForSingleObject(process.hProcess, (timeoutMs == 0) ? INFINITE : timeoutMs);
		if (waitResult == WAIT_TIMEOUT)
		{
			result.bTimedOut = true;
			::TerminateProcess(process.hProcess, 1);
			::WaitForSingleObject(process.hProcess, 5000);
		}

		DWORD exitCode = 1;
		::GetExitCodeProcess(process.hProcess, &exitCode);
		result.ExitCode = (uint32)exitCode;

		::CloseHandle(process.hThread);
		::CloseHandle(process.hProcess);
		return result;
	}

	PString FindVisualStudio2022()
	{
		const PString vswhere = vswherePath();
		if (vswhere.Empty() == true)
		{
			return PString();
		}

		// premake vs2022 로 만든 솔루션(v143)은 VS 2022 MSBuild 로 빌드한다. 없을 때만 가장 새 것.
		PString installation = runAndReadFirstLine(PString::Format("\"%s\" -nologo -version [17.0,18.0) -products * -requires Microsoft.Component.MSBuild -property installationPath", vswhere));
		if (installation.Empty() == true)
		{
			installation = runAndReadFirstLine(PString::Format("\"%s\" -nologo -latest -products * -requires Microsoft.Component.MSBuild -property installationPath", vswhere));
		}

		return installation;
	}

	PString FindMSBuild(const PString& visualStudioPath)
	{
		if (visualStudioPath.Empty() == true)
		{
			return PString();
		}

		const fsys::path msbuild = ToPath(visualStudioPath) / L"MSBuild" / L"Current" / L"Bin" / L"MSBuild.exe";
		std::error_code errorCode;
		return fsys::exists(msbuild, errorCode) ? FromPath(msbuild) : PString();
	}

	PString FindVCRuntimeRedist(const PString& visualStudioPath)
	{
		if (visualStudioPath.Empty() == true)
		{
			return PString();
		}

		const fsys::path redistRoot = ToPath(visualStudioPath) / L"VC" / L"Redist" / L"MSVC";
		std::error_code errorCode;
		if (fsys::is_directory(redistRoot, errorCode) == false)
		{
			return PString();
		}

		std::vector<int32> bestVersion;
		fsys::path bestPath;
		for (const fsys::directory_entry& entry : fsys::directory_iterator(redistRoot, errorCode))
		{
			const std::vector<int32> version = parseVersion(entry.path().filename().wstring());
			if (version.empty() == true)
			{
				continue;
			}

			const fsys::path crtPath = entry.path() / L"x64" / L"Microsoft.VC143.CRT";
			if (fsys::exists(crtPath / L"vcruntime140.dll", errorCode) == false)
			{
				continue;
			}

			if (bestVersion.empty() == true || version > bestVersion)
			{
				bestVersion = version;
				bestPath = crtPath;
			}
		}

		return bestPath.empty() ? PString() : FromPath(bestPath);
	}

	bool ReadImportedDlls(const PString& binaryPath, HList<PString>* outDllNames)
	{
		if (outDllNames == nullptr)
		{
			return false;
		}

		outDllNames->clear();

		std::ifstream stream(ToPath(binaryPath), std::ios::binary);
		if (stream.is_open() == false)
		{
			return false;
		}

		std::vector<uint8> data((std::istreambuf_iterator<char>(stream)), std::istreambuf_iterator<char>());
		if (data.size() < sizeof(IMAGE_DOS_HEADER))
		{
			return false;
		}

		const IMAGE_DOS_HEADER* dosHeader = reinterpret_cast<const IMAGE_DOS_HEADER*>(data.data());
		if (dosHeader->e_magic != IMAGE_DOS_SIGNATURE || dosHeader->e_lfanew <= 0 || (uint64)dosHeader->e_lfanew + sizeof(IMAGE_NT_HEADERS64) > data.size())
		{
			return false;
		}

		const IMAGE_NT_HEADERS64* ntHeaders = reinterpret_cast<const IMAGE_NT_HEADERS64*>(data.data() + dosHeader->e_lfanew);
		if (ntHeaders->Signature != IMAGE_NT_SIGNATURE || ntHeaders->OptionalHeader.Magic != IMAGE_NT_OPTIONAL_HDR64_MAGIC)
		{
			return false;
		}

		const IMAGE_SECTION_HEADER* sections = IMAGE_FIRST_SECTION(ntHeaders);
		const uint32 sectionCount = ntHeaders->FileHeader.NumberOfSections;
		if ((uint64)((const uint8*)(sections + sectionCount) - data.data()) > data.size())
		{
			return false;
		}

		HHashSet<PString> names;

		const IMAGE_DATA_DIRECTORY& importDirectory = ntHeaders->OptionalHeader.DataDirectory[IMAGE_DIRECTORY_ENTRY_IMPORT];
		uint64 offset = (importDirectory.VirtualAddress != 0) ? rvaToOffset(sections, sectionCount, importDirectory.VirtualAddress, data.size()) : 0;
		while (offset != 0 && offset + sizeof(IMAGE_IMPORT_DESCRIPTOR) <= data.size())
		{
			const IMAGE_IMPORT_DESCRIPTOR* descriptor = reinterpret_cast<const IMAGE_IMPORT_DESCRIPTOR*>(data.data() + offset);
			if (descriptor->Name == 0)
			{
				break;
			}

			const uint64 nameOffset = rvaToOffset(sections, sectionCount, descriptor->Name, data.size());
			if (nameOffset != 0)
			{
				names.insert(readCString(data, nameOffset));
			}

			offset += sizeof(IMAGE_IMPORT_DESCRIPTOR);
		}

		const IMAGE_DATA_DIRECTORY& delayDirectory = ntHeaders->OptionalHeader.DataDirectory[IMAGE_DIRECTORY_ENTRY_DELAY_IMPORT];
		offset = (delayDirectory.VirtualAddress != 0) ? rvaToOffset(sections, sectionCount, delayDirectory.VirtualAddress, data.size()) : 0;
		while (offset != 0 && offset + sizeof(IMAGE_DELAYLOAD_DESCRIPTOR) <= data.size())
		{
			const IMAGE_DELAYLOAD_DESCRIPTOR* descriptor = reinterpret_cast<const IMAGE_DELAYLOAD_DESCRIPTOR*>(data.data() + offset);
			if (descriptor->DllNameRVA == 0)
			{
				break;
			}

			const uint64 nameOffset = rvaToOffset(sections, sectionCount, descriptor->DllNameRVA, data.size());
			if (nameOffset != 0)
			{
				names.insert(readCString(data, nameOffset));
			}

			offset += sizeof(IMAGE_DELAYLOAD_DESCRIPTOR);
		}

		for (const PString& name : names)
		{
			outDllNames->push_back(name);
		}

		std::sort(outDllNames->begin(), outDllNames->end(), [](const PString& a, const PString& b)
		{
			return a.GetRawString() < b.GetRawString();
		});

		return true;
	}

	bool IsVCRuntimeDll(const PString& dllName)
	{
		static const char* const prefixes[] = { "vcruntime140", "msvcp140", "concrt140", "vccorlib140", "vcomp140" };
		const HRawString lowerName = toLowerAscii(dllName.GetRawString());
		for (const char* prefix : prefixes)
		{
			if (lowerName.rfind(prefix, 0) == 0)
			{
				return true;
			}
		}

		return false;
	}

	bool IsSystemDll(const PString& dllName)
	{
		const HRawString lowerName = toLowerAscii(dllName.GetRawString());
		if (lowerName.rfind("api-ms-win-", 0) == 0 || lowerName.rfind("ext-ms-", 0) == 0)
		{
			return true;
		}

		if (IsVCRuntimeDll(dllName) == true)
		{
			return false;
		}

		wchar_t systemDirectory[MAX_PATH] = {};
		const UINT length = ::GetSystemDirectoryW(systemDirectory, MAX_PATH);
		if (length == 0 || length >= MAX_PATH)
		{
			return false;
		}

		std::error_code errorCode;
		return fsys::exists(fsys::path(systemDirectory) / toWide(dllName), errorCode);
	}
}
