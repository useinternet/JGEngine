#include "PCH/PCH.h"

#include "Core.h"
#include "Misc/Module.h"
#include "ConsoleCommand/ConsoleCommandGlobalSystem.h"
#include "FileIO/ProjectDescriptor.h"
#include "FileIO/PackageDescriptor.h"
#include "Cook/CookPlatform.h"
#include <chrono>
#include <ctime>
#include <fstream>

// 7단계 콘텐츠 검사(Data 세션 GP-A1 — GameFrameWork Data/ContentCookCheck).
#include "Data/ContentCookCheck.h"

// 게임 쿠킹 · 패키징(DevOps GP-D4, 2026-10-02 — 사용자 요청 "언리얼 쿠킹 및 패키징처럼").
//   JGConsole.exe -runCook -project=<프로젝트 폴더> [-config=ReleaseGame|ConfirmGame] [-output=<폴더>]
//                 [-skipbuild] [-nosmoke] [-smokeframes=300] [-launchmodule=GameHost]
//   (-runCook 은 JGConsole 이 cook.run 으로 바꿔 부르는 별칭. 어느 작업 폴더에서 실행해도 된다)
// 결과 <Project>/Package/<Config>/ = <Name>.exe · Bin/ · Content/ (사용자 결정 D-23: 한 프로세스 · Content 만 · Saved 는 실행 때 생김),
// PDB 는 옆의 <출력>-Symbols/, 로그 · 보고서는 <Project>/Temp/Cook/<Config>/.
// 단계: 1 검사 · 2 생성(배치 2) · 3 빌드(MSBuild) · 4 모듈(runtime_module_info.json) · 5 Bin(+ DLL import 감사 · 앱 로컬 CRT)
//       · 6 exe · 7 Content(고르기 · 검사 · 복사) · 8 서술자(Bin/JGPackage.json) · 9 스모크 실행 · 10 교체 · 보고.
// 작업은 <출력>.staging/ 에서 하고 끝에 바꿔 넣는다 — 중간에 실패해도 이전 패키지가 남는다.
// 패키지는 JG_WITH_EDITOR 0 빌드(ConfirmGame · ReleaseGame)만 된다. 종료 코드: 성공 0, 실패 1.

namespace
{
	namespace fsys = std::filesystem;

	const char* const DEFAULT_CONFIG        = "ReleaseGame";
	const char* const DEFAULT_LAUNCH_MODULE = "GameHost";
	const char* const LAUNCH_DLL            = "JGGameLaunch.dll";
	const char* const BOOTSTRAP_EXE         = "JGGameBootstrap.exe";
	const char* const SYMBOLS_MARKER        = "JGPackageSymbols.txt";
	const int32       DEFAULT_SMOKE_FRAMES  = 300;
	const uint32      SMOKE_TIMEOUT_MS      = 120000;

	// 엔진 Content 기본 제외: 가져오기 원본 · 시험 메시(TempAsset — cookcheck.selftest 가 읽음). 공통: 임시 파일.
	const char* const ENGINE_CONTENT_EXCLUDES[] = { "rawresources/", "tempasset/" };
	const char* const COMMON_EXCLUDES[]         = { "*.tmp", "*.bak", "~*", "thumbs.db", "desktop.ini" };

	struct HCookFile
	{
		fsys::path Source;
		PString    Relative;   // 마운트 기준 상대 경로('/')
	};

	struct HCookContext
	{
		// 옵션
		PString ProjectDirectory;      // 정규화 절대경로('/' · 끝 '/' 없음)
		PString Config = DEFAULT_CONFIG;
		PString OutputDirectory;
		bool    bSkipBuild = false;
		bool    bSmoke     = true;
		int32   SmokeFrames = DEFAULT_SMOKE_FRAMES;
		PString LaunchModule = DEFAULT_LAUNCH_MODULE;

		// 준비된 값
		HProjectDescriptor Project;
		PString EngineDirectory;
		PString ProjectBinDirectory;   // <Project>/Bin/<Config>
		PString LogDirectory;          // <Project>/Temp/Cook/<Config>
		PString StagingDirectory;      // <출력>.staging
		PString SymbolsDirectory;      // <출력>-Symbols
		PString SymbolsStagingDirectory;
		PString VisualStudio;
		HANDLE  LockHandle = INVALID_HANDLE_VALUE;
		HRuntimeModuleInfo Modules;

		std::vector<HCookFile> EngineContent;
		std::vector<HCookFile> GameContent;

		// 보고
		std::vector<HRawString> Report;
		std::vector<HRawString> Warnings;
		uint32 BinaryCount  = 0;
		uint32 ContentCount = 0;
		uint64 ByteCount    = 0;
	};

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

	bool isAscii(const PString& text)
	{
		for (char c : text.GetRawString())
		{
			if ((unsigned char)c >= 0x80)
			{
				return false;
			}
		}

		return true;
	}

	PString windowsPath(const PString& path)
	{
		PString result = path;
		result.ReplaceAll("/", "\\");
		return result;
	}

	PString combine(const PString& a, const char* b)
	{
		return PString(a.GetRawString().c_str()) + "/" + b;
	}

	void warn(HCookContext& context, const PString& message)
	{
		context.Warnings.push_back(message.GetRawString());
		JG_LOG(JGConsole, ELogLevel::Warning, "cook: %s", message);
	}

	// 로그 파일에서 needle 이 든 줄을 최대 maxLines 개 출력한다(빌드 오류 등).
	void printMatchingLines(const PString& logPath, const char* needle, int32 maxLines)
	{
		std::ifstream stream(HCookPlatform::ToPath(logPath));
		HRawString line;
		int32 printed = 0;
		while (std::getline(stream, line) && printed < maxLines)
		{
			if (line.find(needle) != HRawString::npos)
			{
				JG_LOG(JGConsole, ELogLevel::Error, "  %s", PString(line.c_str()));
				++printed;
			}
		}
	}

	void printLastLines(const PString& logPath, int32 lineCount)
	{
		std::ifstream stream(HCookPlatform::ToPath(logPath));
		std::vector<HRawString> lines;
		HRawString line;
		while (std::getline(stream, line))
		{
			lines.push_back(line);
		}

		const size_t first = (lines.size() > (size_t)lineCount) ? lines.size() - lineCount : 0;
		for (size_t i = first; i < lines.size(); ++i)
		{
			JG_LOG(JGConsole, ELogLevel::Error, "  %s", PString(lines[i].c_str()));
		}
	}

	// 지워도 되는 출력 폴더: 없음 · 비어 있음 · markerRelative 가 있는 폴더(이전 쿠킹 결과). 그 밖의 폴더는 건드리지 않는다.
	bool isRemovableOutput(const PString& directory, const char* markerRelative)
	{
		std::error_code errorCode;
		const fsys::path path = HCookPlatform::ToPath(directory);
		if (fsys::exists(path, errorCode) == false)
		{
			return true;
		}

		if (fsys::is_directory(path, errorCode) == false)
		{
			return false;
		}

		if (fsys::is_empty(path, errorCode) == true)
		{
			return true;
		}

		return fsys::exists(path / HCookPlatform::ToPath(markerRelative), errorCode);
	}

	bool removeDirectory(const PString& directory)
	{
		std::error_code errorCode;
		fsys::remove_all(HCookPlatform::ToPath(directory), errorCode);
		return errorCode.value() == 0;
	}

	bool copyFile(const fsys::path& source, const fsys::path& target, uint64* inOutBytes)
	{
		std::error_code errorCode;
		fsys::create_directories(target.parent_path(), errorCode);
		fsys::copy_file(source, target, fsys::copy_options::overwrite_existing, errorCode);
		if (errorCode.value() != 0)
		{
			JG_LOG(JGConsole, ELogLevel::Error, "cook: copy failed %s -> %s: %s", HCookPlatform::FromPath(source), HCookPlatform::FromPath(target), errorCode.message().c_str());
			return false;
		}

		if (inOutBytes != nullptr)
		{
			*inOutBytes += fsys::file_size(target, errorCode);
		}

		return true;
	}

	// 상대 경로(소문자 · '/')가 제외 규칙에 걸리나. 규칙: "폴더/"(접두) · "*.확장자" · "~*"(이름이 ~ 로 시작) · 그 밖은 경로나 파일 이름이 같으면.
	bool isExcluded(const HRawString& relativeLower, const std::vector<HRawString>& rules)
	{
		const size_t slash = relativeLower.find_last_of('/');
		const HRawString fileName = (slash == HRawString::npos) ? relativeLower : relativeLower.substr(slash + 1);

		for (const HRawString& rule : rules)
		{
			if (rule.empty() == true)
			{
				continue;
			}

			if (rule.back() == '/')
			{
				if (relativeLower.rfind(rule, 0) == 0)
				{
					return true;
				}
			}
			else if (rule.size() > 1 && rule[0] == '*' && rule[1] == '.')
			{
				const HRawString extension = rule.substr(1);
				if (relativeLower.size() >= extension.size() && relativeLower.compare(relativeLower.size() - extension.size(), extension.size(), extension) == 0)
				{
					return true;
				}
			}
			else if (rule == "~*")
			{
				if (fileName.empty() == false && fileName[0] == '~')
				{
					return true;
				}
			}
			else if (relativeLower == rule || fileName == rule)
			{
				return true;
			}
		}

		return false;
	}

	void collectFiles(const fsys::path& root, const std::vector<HRawString>& rules, std::vector<HCookFile>* outFiles)
	{
		std::error_code errorCode;
		if (fsys::is_directory(root, errorCode) == false)
		{
			return;
		}

		for (fsys::recursive_directory_iterator it(root, errorCode), end; it != end; it.increment(errorCode))
		{
			if (errorCode.value() != 0)
			{
				break;
			}

			if (it->is_regular_file(errorCode) == false)
			{
				continue;
			}

			PString relative = HCookPlatform::FromPath(fsys::relative(it->path(), root, errorCode));
			relative.ReplaceAll("\\", "/");
			if (isExcluded(toLowerAscii(relative.GetRawString()), rules) == true)
			{
				continue;
			}

			outFiles->push_back(HCookFile{ it->path(), relative });
		}
	}

	std::vector<HRawString> makeRules(std::initializer_list<const char*> fixedRules, const HList<PString>& extraRules)
	{
		std::vector<HRawString> rules;
		for (const char* rule : fixedRules)
		{
			rules.push_back(rule);
		}

		for (const PString& rule : extraRules)
		{
			HRawString lower = toLowerAscii(rule.GetRawString());
			for (char& c : lower)
			{
				if (c == '\\')
				{
					c = '/';
				}
			}

			rules.push_back(lower);
		}

		return rules;
	}

	PString nowText()
	{
		const std::time_t now = std::time(nullptr);
		std::tm local = {};
		localtime_s(&local, &now);
		char buffer[64] = {};
		std::strftime(buffer, sizeof(buffer), "%Y-%m-%dT%H:%M:%S", &local);
		return PString(buffer);
	}

	PString engineCommit(const HCookContext& context)
	{
		const PString outputPath = combine(context.LogDirectory, "git_commit.txt");
		const PString command = PString::Format("git -C \"%s\" rev-parse --short HEAD", windowsPath(context.EngineDirectory));
		const HCookPlatform::HProcessResult result = HCookPlatform::RunProcess(command, PString(), outputPath, 30000);
		if (result.bStarted == false || result.ExitCode != 0)
		{
			return "unknown";
		}

		std::ifstream stream(HCookPlatform::ToPath(outputPath));
		HRawString line;
		std::getline(stream, line);
		while (line.empty() == false && (line.back() == '\r' || line.back() == ' '))
		{
			line.pop_back();
		}

		return line.empty() ? PString("unknown") : PString(line.c_str());
	}

	// ── 1 검사 ──────────────────────────────────────────────────────────────
	bool stepCheck(HCookContext& context)
	{
		context.EngineDirectory = HCookPlatform::NormalizedAbsolute(HFileHelper::EngineDirectory());

		if (context.ProjectDirectory.Empty() == true)
		{
			JG_LOG(JGConsole, ELogLevel::Error, "cook: -project=<game project folder> is required");
			return false;
		}

		if (isAscii(context.ProjectDirectory) == false)
		{
			JG_LOG(JGConsole, ELogLevel::Error, "cook: the project path must be ASCII (premake limit, DevOps P4): %s", context.ProjectDirectory);
			return false;
		}

		if (HProjectDescriptor::Load(context.ProjectDirectory + "/", &context.Project) == false)
		{
			JG_LOG(JGConsole, ELogLevel::Error, "cook: no %s in %s", HProjectDescriptor::FileExtension, context.ProjectDirectory);
			return false;
		}

		PString boundEngine;
		if (context.Project.ResolveEngineRoot(context.ProjectDirectory + "/", &boundEngine) == false)
		{
			JG_LOG(JGConsole, ELogLevel::Error, "cook: project %s has no EngineRoot", context.Project.Name);
			return false;
		}

		if (toLowerAscii(HCookPlatform::NormalizedAbsolute(boundEngine).GetRawString()) != toLowerAscii(context.EngineDirectory.GetRawString()))
		{
			JG_LOG(JGConsole, ELogLevel::Error, "cook: project %s belongs to engine %s, but this JGConsole is in %s", context.Project.Name, boundEngine, context.EngineDirectory);
			return false;
		}

		if (context.Config != "ReleaseGame" && context.Config != "ConfirmGame")
		{
			JG_LOG(JGConsole, ELogLevel::Error, "cook: -config=%s - packages are built only from ConfirmGame or ReleaseGame (JG_WITH_EDITOR 0)", context.Config);
			return false;
		}

		if (context.OutputDirectory.Empty() == true)
		{
			context.OutputDirectory = context.ProjectDirectory + "/Package/" + context.Config;
		}
		context.OutputDirectory = HCookPlatform::NormalizedAbsolute(context.OutputDirectory);

		if (isAscii(context.OutputDirectory) == false)
		{
			JG_LOG(JGConsole, ELogLevel::Error, "cook: the output path must be ASCII: %s", context.OutputDirectory);
			return false;
		}

		context.StagingDirectory        = context.OutputDirectory + ".staging";
		context.SymbolsDirectory        = context.OutputDirectory + "-Symbols";
		context.SymbolsStagingDirectory = context.SymbolsDirectory + ".staging";
		context.ProjectBinDirectory     = context.ProjectDirectory + "/Bin/" + context.Config;
		context.LogDirectory            = context.ProjectDirectory + "/Temp/Cook/" + context.Config;

		// 출력 폴더 가드 — 엔진 · 프로젝트 · 드라이브 루트이거나 그 조상이면 거부, 이전 쿠킹 결과가 아닌 폴더는 지우지 않는다.
		const PString outputs[] = { context.OutputDirectory, context.StagingDirectory, context.SymbolsDirectory, context.SymbolsStagingDirectory };
		for (const PString& output : outputs)
		{
			if (output.Length() <= 3 || HCookPlatform::IsSameOrAncestor(output, context.ProjectDirectory) == true || HCookPlatform::IsSameOrAncestor(output, context.EngineDirectory) == true)
			{
				JG_LOG(JGConsole, ELogLevel::Error, "cook: refusing output %s (drive root, or the engine/project folder or its parent)", output);
				return false;
			}
		}

		if (isRemovableOutput(context.OutputDirectory, "Bin/JGPackage.json") == false || isRemovableOutput(context.StagingDirectory, "Bin/JGPackage.json") == false)
		{
			JG_LOG(JGConsole, ELogLevel::Error, "cook: %s exists and is not a previous package - choose another -output=", context.OutputDirectory);
			return false;
		}

		if (isRemovableOutput(context.SymbolsDirectory, SYMBOLS_MARKER) == false || isRemovableOutput(context.SymbolsStagingDirectory, SYMBOLS_MARKER) == false)
		{
			JG_LOG(JGConsole, ELogLevel::Error, "cook: %s exists and is not a previous symbols folder", context.SymbolsDirectory);
			return false;
		}

		std::error_code errorCode;
		fsys::create_directories(HCookPlatform::ToPath(context.LogDirectory), errorCode);

		// 같은 프로젝트를 동시에 쿠킹하지 않는다. 닫히면(프로세스가 죽어도) 지워지는 잠금 파일.
		const PString lockPath = context.ProjectDirectory + "/Temp/Cook/cook.lock";
		context.LockHandle = ::CreateFileW(HCookPlatform::ToPath(lockPath).c_str(), GENERIC_WRITE, 0, nullptr, OPEN_ALWAYS, FILE_ATTRIBUTE_NORMAL | FILE_FLAG_DELETE_ON_CLOSE, nullptr);
		if (context.LockHandle == INVALID_HANDLE_VALUE)
		{
			JG_LOG(JGConsole, ELogLevel::Error, "cook: another cook of this project is running (%s)", lockPath);
			return false;
		}

		removeDirectory(context.StagingDirectory);
		removeDirectory(context.SymbolsStagingDirectory);

		context.Report.push_back(PString::Format("project  %s (%s)", context.Project.Name, context.ProjectDirectory).GetRawString());
		context.Report.push_back(PString::Format("engine   %s", context.EngineDirectory).GetRawString());
		context.Report.push_back(PString::Format("config   %s, output %s", context.Config, context.OutputDirectory).GetRawString());
		return true;
	}

	// ── 2 생성 ──────────────────────────────────────────────────────────────
	bool stepGenerate(HCookContext& context)
	{
		if (context.bSkipBuild == true)
		{
			context.Report.push_back("generate skipped (-skipbuild)");
			return true;
		}

		const PString batch = windowsPath(context.EngineDirectory + "/Build/BatchFiles/GenerateGameProjectFiles.bat");
		const PString command = PString::Format("cmd.exe /d /c \"\"%s\" \"%s\"\"", batch, windowsPath(context.ProjectDirectory));
		const PString logPath = combine(context.LogDirectory, "generate.log");
		const HCookPlatform::HProcessResult result = HCookPlatform::RunProcess(command, PString(), logPath, 600000);
		if (result.bStarted == false || result.bTimedOut == true || result.ExitCode != 0)
		{
			JG_LOG(JGConsole, ELogLevel::Error, "cook: GenerateGameProjectFiles failed (exit %u) - %s", result.ExitCode, logPath);
			printLastLines(logPath, 15);
			return false;
		}

		return true;
	}

	// ── 3 빌드 ──────────────────────────────────────────────────────────────
	bool stepBuild(HCookContext& context)
	{
		context.VisualStudio = HCookPlatform::FindVisualStudio2022();
		if (context.bSkipBuild == true)
		{
			context.Report.push_back("build skipped (-skipbuild)");
			return true;
		}

		const PString msbuild = HCookPlatform::FindMSBuild(context.VisualStudio);
		if (msbuild.Empty() == true)
		{
			JG_LOG(JGConsole, ELogLevel::Error, "cook: MSBuild (Visual Studio 2022) not found");
			return false;
		}

		const PString solution = windowsPath(context.ProjectDirectory + "/" + context.Project.Name + ".sln");
		const PString command = PString::Format("\"%s\" \"%s\" -p:Configuration=%s -p:Platform=x64 -m -nologo -v:minimal -nr:false", msbuild, solution, context.Config);
		const PString logPath = combine(context.LogDirectory, "build.log");
		const HCookPlatform::HProcessResult result = HCookPlatform::RunProcess(command, context.ProjectDirectory, logPath, 3600000);
		if (result.bStarted == false || result.bTimedOut == true || result.ExitCode != 0)
		{
			JG_LOG(JGConsole, ELogLevel::Error, "cook: build failed (exit %u) - %s", result.ExitCode, logPath);
			printMatchingLines(logPath, ": error", 20);
			return false;
		}

		context.Report.push_back(PString::Format("msbuild  %s", msbuild).GetRawString());
		return true;
	}

	// ── 4 모듈 ──────────────────────────────────────────────────────────────
	bool stepModules(HCookContext& context)
	{
		const PString path = context.ProjectDirectory + "/Temp/CodeGen/" + HRuntimeModuleInfo::FileName;
		if (HRuntimeModuleInfo::Load(path, &context.Modules) == false)
		{
			JG_LOG(JGConsole, ELogLevel::Error, "cook: %s missing - run the project's GenerateProjectFiles.bat (or cook without -skipbuild)", path);
			return false;
		}

		if (context.Modules.EditorDependencies.empty() == false)
		{
			for (const PString& dependency : context.Modules.EditorDependencies)
			{
				JG_LOG(JGConsole, ELogLevel::Error, "cook: runtime module depends on an editor module: %s", dependency);
			}

			return false;
		}

		for (const PString& dependency : context.Modules.DevelopmentDependencies)
		{
			warn(context, PString::Format("runtime module depends on a development module (%s) - ImGui ships in the package", dependency));
		}

		bool bHasLaunchModule = false;
		for (const PString& moduleName : context.Modules.RuntimeModules)
		{
			if (moduleName == context.LaunchModule)
			{
				bHasLaunchModule = true;
			}
		}

		if (bHasLaunchModule == false)
		{
			JG_LOG(JGConsole, ELogLevel::Error, "cook: launch module %s is not a runtime module of %s", context.LaunchModule, context.Project.Name);
			return false;
		}

		if (context.Modules.GameModules.empty() == true)
		{
			warn(context, "the project has no GameModules");
		}

		PString moduleList;
		for (const PString& moduleName : context.Modules.RuntimeModules)
		{
			moduleList.Append(moduleList.Empty() ? "" : ", ").Append(moduleName);
		}
		context.Report.push_back(PString::Format("modules  %s", moduleList).GetRawString());
		return true;
	}

	// ── 5 Bin ───────────────────────────────────────────────────────────────
	bool stepBinaries(HCookContext& context)
	{
		const fsys::path sourceBin  = HCookPlatform::ToPath(context.ProjectBinDirectory);
		const fsys::path stagingBin = HCookPlatform::ToPath(context.StagingDirectory) / L"Bin";
		const fsys::path symbols    = HCookPlatform::ToPath(context.SymbolsStagingDirectory);

		std::error_code errorCode;
		fsys::create_directories(stagingBin, errorCode);
		fsys::create_directories(symbols, errorCode);
		{
			// Log.h 의 operator<<(OStream&, const PString&) 템플릿과 겹치지 않게 std::ostream 으로 쓴다.
			std::ofstream markerFile(symbols / HCookPlatform::ToPath(SYMBOLS_MARKER));
			std::ostream& marker = markerFile;
			marker <<"PDB files of the game package next to this folder (not shipped). DevOps GP-D4\n";
		}

		HHashSet<PString> staged;    // 소문자 DLL 이름
		std::vector<fsys::path> auditQueue;

		auto stageBinary = [&](const PString& fileName, bool bRequired) -> bool
		{
			const fsys::path source = sourceBin / HCookPlatform::ToPath(fileName);
			if (fsys::exists(source, errorCode) == false)
			{
				if (bRequired == true)
				{
					JG_LOG(JGConsole, ELogLevel::Error, "cook: %s not found in %s (build %s first)", fileName, context.ProjectBinDirectory, context.Config);
				}
				return false;
			}

			const fsys::path target = stagingBin / HCookPlatform::ToPath(fileName);
			if (copyFile(source, target, &context.ByteCount) == false)
			{
				return false;
			}

			staged.insert(PString(toLowerAscii(fileName.GetRawString()).c_str()));
			auditQueue.push_back(target);
			++context.BinaryCount;

			fsys::path pdb = source;
			pdb.replace_extension(L".pdb");
			if (fsys::exists(pdb, errorCode) == true)
			{
				copyFile(pdb, symbols / pdb.filename(), nullptr);
			}

			return true;
		};

		for (const PString& moduleName : context.Modules.RuntimeModules)
		{
			if (stageBinary(moduleName + ".dll", true) == false)
			{
				return false;
			}
		}

		if (stageBinary(LAUNCH_DLL, true) == false)
		{
			return false;
		}

		// 부트스트랩(6단계에서 <Name>.exe 가 됨)도 import 를 본다.
		const fsys::path bootstrap = sourceBin / BOOTSTRAP_EXE;
		if (fsys::exists(bootstrap, errorCode) == false)
		{
			JG_LOG(JGConsole, ELogLevel::Error, "cook: %s not found in %s", BOOTSTRAP_EXE, context.ProjectBinDirectory);
			return false;
		}
		auditQueue.push_back(bootstrap);

		// import 감사: 배치한 바이너리가 쓰는 DLL 이 모두 패키지 · Windows 에 있어야 한다. 개발 PC 는 System32 에 VC 런타임이 있어
		// 빠뜨려도 실행되므로 스모크로는 못 잡는다 — 여기서 앱 로컬 CRT 와 3rd-party 를 채운다.
		PString redist;
		for (size_t i = 0; i < auditQueue.size(); ++i)
		{
			const fsys::path binary = auditQueue[i];
			HList<PString> imports;
			if (HCookPlatform::ReadImportedDlls(HCookPlatform::FromPath(binary), &imports) == false)
			{
				JG_LOG(JGConsole, ELogLevel::Error, "cook: cannot read the import table of %s", HCookPlatform::FromPath(binary));
				return false;
			}

			for (const PString& dllName : imports)
			{
				if (staged.find(dllName) != staged.end() || HCookPlatform::IsSystemDll(dllName) == true)
				{
					continue;
				}

				fsys::path source;
				if (HCookPlatform::IsVCRuntimeDll(dllName) == true)
				{
					if (redist.Empty() == true)
					{
						if (context.VisualStudio.Empty() == true)
						{
							context.VisualStudio = HCookPlatform::FindVisualStudio2022();
						}
						redist = HCookPlatform::FindVCRuntimeRedist(context.VisualStudio);
						if (redist.Empty() == true)
						{
							JG_LOG(JGConsole, ELogLevel::Error, "cook: VC++ runtime redist (VC/Redist/MSVC/*/x64/Microsoft.VC143.CRT) not found");
							return false;
						}
						context.Report.push_back(PString::Format("crt      %s", redist).GetRawString());
					}

					source = HCookPlatform::ToPath(redist) / HCookPlatform::ToPath(dllName);
				}
				else
				{
					source = sourceBin / HCookPlatform::ToPath(dllName);
					if (fsys::exists(source, errorCode) == true)
					{
						warn(context, PString::Format("3rd-party DLL %s copied from %s (needed by %s)", dllName, context.ProjectBinDirectory, HCookPlatform::FromPath(binary.filename())));
					}
				}

				if (fsys::exists(source, errorCode) == false)
				{
					JG_LOG(JGConsole, ELogLevel::Error, "cook: %s needs %s, which is neither in the package, Windows, nor %s", HCookPlatform::FromPath(binary.filename()), dllName, context.ProjectBinDirectory);
					return false;
				}

				const fsys::path target = stagingBin / HCookPlatform::ToPath(dllName);
				if (copyFile(source, target, &context.ByteCount) == false)
				{
					return false;
				}

				staged.insert(dllName);
				auditQueue.push_back(target);
				++context.BinaryCount;
			}
		}

		return true;
	}

	// ── 6 exe ───────────────────────────────────────────────────────────────
	bool stepExecutable(HCookContext& context)
	{
		const fsys::path source = HCookPlatform::ToPath(context.ProjectBinDirectory) / BOOTSTRAP_EXE;
		const fsys::path target = HCookPlatform::ToPath(context.StagingDirectory) / HCookPlatform::ToPath(context.Project.Name + ".exe");
		if (copyFile(source, target, &context.ByteCount) == false)
		{
			return false;
		}

		++context.BinaryCount;
		return true;
	}

	// ── 7 Content ───────────────────────────────────────────────────────────
	bool stepContent(HCookContext& context)
	{
		// Content 만 패키징한다(사용자 결정 D-23 ③). 엔진 Content → Content/JGEngine, 게임 Content → Content/JGGame(에셋 경로 토큰과 같은 이름).
		const fsys::path engineContent = HCookPlatform::ToPath(context.EngineDirectory + "/Content");
		const fsys::path gameContent   = HCookPlatform::ToPath(context.ProjectDirectory + "/Content");

		collectFiles(engineContent, makeRules({ "rawresources/", "tempasset/", "*.tmp", "*.bak", "~*", "thumbs.db", "desktop.ini" }, HList<PString>()), &context.EngineContent);
		collectFiles(gameContent, makeRules({ "*.tmp", "*.bak", "~*", "thumbs.db", "desktop.ini" }, context.Project.Package.ExcludeContent), &context.GameContent);

		// 셰이더는 엔진 Content 의 Shaders/ 에 있다(Graphics GP-G4) — 위 엔진 Content 와 함께 Content/JGEngine/Shaders 로 들어간다.
		// 없으면 패키지가 셰이더를 못 찾아 그리지 못하므로 여기서 멈춘다.
		std::error_code errorCode;
		if (fsys::is_directory(engineContent / L"Shaders", errorCode) == false)
		{
			JG_LOG(JGConsole, ELogLevel::Error, "cook: %s/Content/Shaders not found - the engine shaders must be in engine Content", context.EngineDirectory);
			return false;
		}

		HList<PString> files;
		for (const HCookFile& file : context.EngineContent)
		{
			files.push_back(HCookPlatform::FromPath(file.Source));
		}
		for (const HCookFile& file : context.GameContent)
		{
			files.push_back(HCookPlatform::FromPath(file.Source));
		}

		// 토큰은 GameFrameWork Asset/AssetDefines.h 의 JG_ASSET_ENGINE/GAME_PATH_RECOGNITION_TOEKN 과 같은 값이다.
		// Asset 이 따로 모듈이던 때 JGConsole 이 그 헤더를 include 하지 못해(include 경로는 직접 의존 Core · GameFrameWork 만 — DevOps C1-H1) 값을 직접 쓴 것이다.
		HList<HPair<PString, PString>> mounts;
		mounts.push_back(HPair<PString, PString>(PString("/JGEngine/"), HCookPlatform::NormalizedAbsolute(context.EngineDirectory + "/Content") + "/"));
		mounts.push_back(HPair<PString, PString>(PString("/JGGame/"), HCookPlatform::NormalizedAbsolute(context.ProjectDirectory + "/Content") + "/"));

		HList<PString> errors;
		HList<PString> warnings;
		const bool bValid = ValidateContentForCook(files, mounts, errors, warnings);
		for (const PString& warning : warnings)
		{
			warn(context, warning);
		}

		if (bValid == false)
		{
			for (const PString& error : errors)
			{
				JG_LOG(JGConsole, ELogLevel::Error, "cook: content: %s", error);
			}

			return false;
		}

		const fsys::path stagingContent = HCookPlatform::ToPath(context.StagingDirectory) / L"Content";
		for (const HCookFile& file : context.EngineContent)
		{
			if (copyFile(file.Source, stagingContent / L"JGEngine" / HCookPlatform::ToPath(file.Relative), &context.ByteCount) == false)
			{
				return false;
			}
		}

		for (const HCookFile& file : context.GameContent)
		{
			if (copyFile(file.Source, stagingContent / L"JGGame" / HCookPlatform::ToPath(file.Relative), &context.ByteCount) == false)
			{
				return false;
			}
		}

		// 마운트 폴더는 비어 있어도 둔다(엔진이 그 경로를 마운트로 본다).
		fsys::create_directories(stagingContent / L"JGEngine", errorCode);
		fsys::create_directories(stagingContent / L"JGGame", errorCode);

		context.ContentCount = (uint32)(context.EngineContent.size() + context.GameContent.size());
		context.Report.push_back(PString::Format("content  engine %d files, game %d files", (int32)context.EngineContent.size(), (int32)context.GameContent.size()).GetRawString());
		return true;
	}

	// ── 8 서술자 ────────────────────────────────────────────────────────────
	bool stepDescriptor(HCookContext& context)
	{
		HPackageDescriptor package;
		package.Name           = context.Project.Name;
		package.Version        = context.Project.Package.Version;
		package.BuildConfig    = context.Config;
		package.EngineCommit   = engineCommit(context);
		package.CookedAt       = nowText();
		package.LaunchModule   = context.LaunchModule;
		package.RuntimeModules = context.Modules.RuntimeModules;
		package.GameModules    = context.Modules.GameModules;
		package.Window         = context.Project.Package.Window;
		if (package.Window.Title.Empty() == true)
		{
			package.Window.Title = context.Project.Name;
		}

		if (package.Save(context.StagingDirectory + "/") == false)
		{
			JG_LOG(JGConsole, ELogLevel::Error, "cook: fail write Bin/%s", HPackageDescriptor::FileName);
			return false;
		}

		context.Report.push_back(PString::Format("package  %s %s, engine %s, cooked %s, launch %s, window %dx%d %s", package.Name, package.Version, package.EngineCommit, package.CookedAt, package.LaunchModule, package.Window.Width, package.Window.Height, package.Window.Mode).GetRawString());
		return true;
	}

	// ── 9 스모크 ────────────────────────────────────────────────────────────
	bool stepSmoke(HCookContext& context)
	{
		if (context.bSmoke == false)
		{
			context.Report.push_back("smoke    skipped (-nosmoke)");
			return true;
		}

		const PString exePath = context.StagingDirectory + "/" + context.Project.Name + ".exe";
		const PString command = PString::Format("\"%s\" -smoke=%d", windowsPath(exePath), context.SmokeFrames);
		const PString stdoutPath = combine(context.LogDirectory, "smoke_stdout.log");
		const HCookPlatform::HProcessResult result = HCookPlatform::RunProcess(command, context.StagingDirectory, stdoutPath, SMOKE_TIMEOUT_MS);

		const PString gameLog = context.StagingDirectory + "/Saved/Logs/" + context.Project.Name + ".log";
		const PString smokeLog = combine(context.LogDirectory, "smoke.log");
		std::error_code errorCode;
		fsys::copy_file(HCookPlatform::ToPath(gameLog), HCookPlatform::ToPath(smokeLog), fsys::copy_options::overwrite_existing, errorCode);

		int32 errorLines = 0;
		int32 outsideReads = 0;
		{
			std::ifstream stream(HCookPlatform::ToPath(gameLog));
			HRawString line;
			while (std::getline(stream, line))
			{
				if (line.find("[error]") != HRawString::npos || line.find("[critical]") != HRawString::npos)
				{
					++errorLines;
					if (errorLines <= 20)
					{
						JG_LOG(JGConsole, ELogLevel::Error, "  %s", PString(line.c_str()));
					}
				}

				if (line.find("Package reads outside") != HRawString::npos)
				{
					++outsideReads;
				}
			}
		}

		// 스모크가 만든 Saved/ 는 패키지에 남기지 않는다.
		removeDirectory(context.StagingDirectory + "/Saved");

		context.Report.push_back(PString::Format("smoke    %d frames, exit %u%s, error lines %d, reads outside Content %d - %s", context.SmokeFrames, result.ExitCode, result.bTimedOut ? " (timed out)" : "", errorLines, outsideReads, smokeLog).GetRawString());

		if (outsideReads > 0)
		{
			warn(context, PString::Format("the game reads %d folder(s) outside Content/Saved/Bin - see %s", outsideReads, smokeLog));
		}

		if (result.bStarted == false || result.bTimedOut == true || result.ExitCode != 0 || errorLines > 0)
		{
			JG_LOG(JGConsole, ELogLevel::Error, "cook: smoke run failed (started %s, timed out %s, exit %u, error lines %d) - %s", result.bStarted ? "yes" : "no", result.bTimedOut ? "yes" : "no", result.ExitCode, errorLines, smokeLog);
			return false;
		}

		return true;
	}

	// ── 10 교체 ─────────────────────────────────────────────────────────────
	bool stepFinish(HCookContext& context)
	{
		std::error_code errorCode;
		if (removeDirectory(context.OutputDirectory) == false || removeDirectory(context.SymbolsDirectory) == false)
		{
			JG_LOG(JGConsole, ELogLevel::Error, "cook: cannot remove the previous package %s (is the game running?)", context.OutputDirectory);
			return false;
		}

		fsys::create_directories(HCookPlatform::ToPath(context.OutputDirectory).parent_path(), errorCode);
		fsys::rename(HCookPlatform::ToPath(context.StagingDirectory), HCookPlatform::ToPath(context.OutputDirectory), errorCode);
		if (errorCode.value() != 0)
		{
			JG_LOG(JGConsole, ELogLevel::Error, "cook: cannot move %s to %s: %s", context.StagingDirectory, context.OutputDirectory, errorCode.message().c_str());
			return false;
		}

		fsys::rename(HCookPlatform::ToPath(context.SymbolsStagingDirectory), HCookPlatform::ToPath(context.SymbolsDirectory), errorCode);
		return true;
	}

	void writeReport(const HCookContext& context, bool bSucceeded, double totalSeconds)
	{
		if (context.LogDirectory.Empty() == true)
		{
			return;
		}

		// Log.h 의 operator<<(OStream&, const PString&) 템플릿과 겹치지 않게 std::ostream 으로 쓴다.
		std::ofstream reportFile(HCookPlatform::ToPath(combine(context.LogDirectory, "report.txt")));
		std::ostream& report = reportFile;
		report << "JGEngine cook report (" << nowText().GetRawString() << ") - " << (bSucceeded ? "OK" : "FAILED") << "\n";
		for (const HRawString& line : context.Report)
		{
			report << line << "\n";
		}

		report << PString::Format("total    %.1f s, binaries %u, content files %u, %.1f MB", totalSeconds, context.BinaryCount, context.ContentCount, (double)context.ByteCount / (1024.0 * 1024.0)).GetRawString() << "\n";
		report << "warnings " << context.Warnings.size() << "\n";
		for (const HRawString& warning : context.Warnings)
		{
			report << "  " << warning << "\n";
		}
	}

	struct HCookStep
	{
		const char* Name;
		bool (*Run)(HCookContext&);
	};

	bool executeCook(const HConsoleCommandArgs& args)
	{
		HCookContext context;
		PString value;
		if (args.TryGetString("project", value) == true)
		{
			context.ProjectDirectory = HCookPlatform::NormalizedAbsolute(value);
		}
		if (args.TryGetString("config", value) == true)
		{
			context.Config = value;
		}
		if (args.TryGetString("output", value) == true)
		{
			context.OutputDirectory = value;
		}
		if (args.TryGetString("launchmodule", value) == true)
		{
			context.LaunchModule = value;
		}
		int32 frames = 0;
		if (args.TryGetInt("smokeframes", frames) == true && frames > 0)
		{
			context.SmokeFrames = frames;
		}
		context.bSkipBuild = args.Has("skipbuild");
		context.bSmoke     = (args.Has("nosmoke") == false);

		static const HCookStep steps[] =
		{
			{ "check",      &stepCheck },
			{ "generate",   &stepGenerate },
			{ "build",      &stepBuild },
			{ "modules",    &stepModules },
			{ "binaries",   &stepBinaries },
			{ "executable", &stepExecutable },
			{ "content",    &stepContent },
			{ "descriptor", &stepDescriptor },
			{ "smoke",      &stepSmoke },
			{ "finish",     &stepFinish },
		};
		const int32 stepCount = (int32)(sizeof(steps) / sizeof(steps[0]));

		const auto cookStart = std::chrono::steady_clock::now();
		bool bSucceeded = true;
		for (int32 i = 0; i < stepCount; ++i)
		{
			const auto stepStart = std::chrono::steady_clock::now();
			JG_LOG(JGConsole, ELogLevel::Info, "[Cook %d/%d] %s ...", i + 1, stepCount, steps[i].Name);
			const bool bStepOk = steps[i].Run(context);
			const double seconds = std::chrono::duration<double>(std::chrono::steady_clock::now() - stepStart).count();
			context.Report.push_back(PString::Format("step %2d  %-10s %s %.1f s", i + 1, steps[i].Name, bStepOk ? "ok    " : "FAILED", seconds).GetRawString());
			if (bStepOk == false)
			{
				JG_LOG(JGConsole, ELogLevel::Error, "[Cook %d/%d] %s FAILED (%.1f s)", i + 1, stepCount, steps[i].Name, seconds);
				bSucceeded = false;
				break;
			}

			JG_LOG(JGConsole, ELogLevel::Info, "[Cook %d/%d] %s ok (%.1f s)", i + 1, stepCount, steps[i].Name, seconds);
		}

		const double totalSeconds = std::chrono::duration<double>(std::chrono::steady_clock::now() - cookStart).count();
		if (bSucceeded == false && context.StagingDirectory.Empty() == false)
		{
			// 실패하면 만들던 것만 지운다. 이전 패키지는 그대로다.
			removeDirectory(context.StagingDirectory);
			removeDirectory(context.SymbolsStagingDirectory);
		}

		writeReport(context, bSucceeded, totalSeconds);
		if (context.LockHandle != INVALID_HANDLE_VALUE)
		{
			::CloseHandle(context.LockHandle);
		}

		if (bSucceeded == true)
		{
			JG_LOG(JGConsole, ELogLevel::Info, "cook.run: OK %s (%.1f s, %u binaries, %u content files, %.1f MB, %d warning(s)) - report %s/report.txt",
				context.OutputDirectory, totalSeconds, context.BinaryCount, context.ContentCount, (double)context.ByteCount / (1024.0 * 1024.0), (int32)context.Warnings.size(), context.LogDirectory);
		}
		else
		{
			JG_LOG(JGConsole, ELogLevel::Error, "cook.run: FAILED (%.1f s) - report %s/report.txt", totalSeconds, context.LogDirectory);
		}

		return bSucceeded;
	}

	HAutoConsoleCommand CookRunCommand(
		"cook.run",
		"cook.run -project=<dir> [-config=ReleaseGame|ConfirmGame] [-output=<dir>] [-skipbuild] [-nosmoke] [-smokeframes=300] [-launchmodule=GameHost]",
		"Cook and package a game project into <Project>/Package/<Config>: <Name>.exe, Bin/, Content/ (alias: JGConsole.exe -runCook ...)",
		&executeCook);
}
