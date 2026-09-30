#include "PCH/PCH.h"
#include "BuildTool.h"
#include "Misc/Module.h"

#include<stdio.h>

const PString& PBuildTool::BuildToolDirectory()
{
	static PString buildToolDir;
	if(buildToolDir.Empty() == true)
	{
		HFileHelper::CombinePath(HFileHelper::EngineProgramsSourceDirectory(), "JGBuildTool", &buildToolDir);
	}
	
	return buildToolDir;
}

PBuildTool::PBuildTool(const PArguments& args) : _arguments(args) {}

bool PBuildTool::Run()
{
	const PArguments& arguments = getArguments();

	// Step 1. Engine Module ���� ���� 
	JG_LOG(BuildTool, ELogLevel::Info, "Step 1. Engine Collection Module Infos..");
	if (collectionModuleInfos(arguments.EngineWorkDirectory, arguments.EngineWorkCategories, true, _engineModuleInfoMap) == false)
	{
		return false;
	}

	// Step 2. User Module ���� ���� 
	JG_LOG(BuildTool, ELogLevel::Info, "Step 2. User Collection Module Infos..");
	if (collectionModuleInfos(arguments.UserWorkDirectory, arguments.UserWorkCategories, false, _userModuleInfoMap) == false)
	{
		return false;
	}

	// Step 3. lua �ڵ� �ۼ�
	JG_LOG(BuildTool, ELogLevel::Info, "Step 3. Generate Build Script..");
	if (generateBuildScript() == false)
	{
		return false;
	}

	// Step 4. 
	JG_LOG(BuildTool, ELogLevel::Info, "Step 4. Insert Include PCH..");
	insertIncludePCHHeaderCode();

	// Step 5. make Build
	JG_LOG(BuildTool, ELogLevel::Info, "Step 5. Make Project Files..");
	if (HFileHelper::IsProjectMode() == true)
	{
		if (makeGameProjectFiles() == false)
		{
			return false;
		}

		if (copyThirdPartyBinaries() == false)
		{
			return false;
		}
	}
	else if (makeProjectFiles() == false)
	{
		return false;
	}

	{
		HModuleSystemInfo moduleSysInfo;
		for (const HPair<const PString, HList<PModuleInfo>>& _pair : _engineModuleInfoMap)
		{
			for (const PModuleInfo& info : _pair.second)
			{
				if (info.ModuleFormat == "SharedLib")
				{
					moduleSysInfo.CodeGenableModuleSet.insert(info.ModuleName);
				}
			}
			
		}
		for (const HPair<const PString, HList<PModuleInfo>>& _pair : _userModuleInfoMap)
		{
			for (const PModuleInfo& info : _pair.second)
			{
				if (info.ModuleFormat == "SharedLib")
				{
					moduleSysInfo.CodeGenableModuleSet.insert(info.ModuleName);
				}
			}
		}

		if (HModuleSystemInfo::Set(moduleSysInfo) == true)
		{
			for (const PString& moduleName : moduleSysInfo.CodeGenableModuleSet)
			{
				JG_LOG(BuildTool, ELogLevel::Info, "Recognize CodeGenable Module : %s", moduleName);
			}
		}
	}

	JG_LOG(BuildTool, ELogLevel::Info, "Compelete Build Tool Run..");
	return true;
}

const PArguments& PBuildTool::getArguments() const
{
	return _arguments;
}

bool PBuildTool::collectionModuleInfos(const PString& workDir, const HHashSet<PString>& workCategories, bool bEngineModule, HHashMap<PString, HList<PModuleInfo>>& outModuleInfoMap)
{
	if (HFileHelper::Exists(workDir) == false)
	{
		JG_LOG(BuildTool, ELogLevel::Critical, "work directory doesn't exist.");

		return false;
	}

	HList<PString> workCategoies;
	HFileHelper::FileListInDirectory(workDir, &workCategoies);

	for (const PString& categoryPath : workCategoies)
	{
		PString categoryName;
		HFileHelper::FileName(categoryPath, &categoryName);

		// 카테고리가 아닌 항목은 먼저 건너뛴다. 게임 프로젝트 루트에는 .jgproject · 배치 파일 같은 파일이 있다.
		if (workCategories.find(categoryName) == workCategories.end())
		{
			continue;
		}

		if (HFileHelper::Exists(categoryPath) == false)
		{
			JG_LOG(BuildTool, ELogLevel::Critical, "work category(%s) doesn't exist.", categoryName);
			continue;
		}

		if (HFileHelper::IsDirectory(categoryPath) == false)
		{
			JG_LOG(BuildTool, ELogLevel::Critical, "work category(%s) must directory file", categoryName);
			continue;
		}

		collectionModuleInfosInternal(categoryName, categoryPath, bEngineModule, outModuleInfoMap);
	}

	uint64 totalCount = 0;
	HHashMap<PString, uint64> moduleCountMap;

	for (const HPair<PString, HList<PModuleInfo>>& pair : outModuleInfoMap)
	{
		moduleCountMap[pair.first] = 0;

		JG_LOG(BuildTool, ELogLevel::Trace, "----------------------------------------------------");
		JG_LOG(BuildTool, ELogLevel::Trace, "%s Category Module List", pair.first);
		JG_LOG(BuildTool, ELogLevel::Trace, "----------------------------------------------------");
		for (const PModuleInfo& moduleInfo : pair.second)
		{
			JG_LOG(BuildTool, ELogLevel::Trace, "%s", moduleInfo.ModuleName);
			moduleCountMap[pair.first] += 1;
			totalCount += 1;
		}

		JG_LOG(BuildTool, ELogLevel::Trace, "----------------------------------------------------");
	}

	for (const HPair<PString, uint64>& pair : moduleCountMap)
	{
		JG_LOG(BuildTool, ELogLevel::Trace, "%s: %d", pair.first, pair.second);
	}

	JG_LOG(BuildTool, ELogLevel::Trace, "Total: %d", totalCount);
	JG_LOG(BuildTool, ELogLevel::Trace, "----------------------------------------------------");
	return true;
}

void PBuildTool::collectionModuleInfosInternal(const PString& categoryName, const PString& inCategoryPath, bool bEngineModule, HHashMap<PString, HList<PModuleInfo>>& outModuleInfoMap)
{
	HList<PString> modulePathList;
	HFileHelper::FileListInDirectory(inCategoryPath, &modulePathList, true);

	PString moduleInfoFileName;
	PString moduleName;

	for (const PString& modulePath : modulePathList)
	{
		HFileHelper::FileName(modulePath, &moduleName);

		if (HFileHelper::Exists(modulePath) == false)
		{
			continue;
		}

		if (HFileHelper::IsDirectory(modulePath) == false)
		{
			continue;
		}

		PModuleInfo moduleInfo;
		if (findModuleInfo(modulePath, &moduleInfo) == false)
		{
			continue;
		}

		moduleInfo.bEngineModule = bEngineModule;

		outModuleInfoMap[categoryName].push_back(moduleInfo);
		_moduleInfoPool[moduleName] = moduleInfo;
	}
}

bool PBuildTool::generateBuildScript()
{
	PString engineModuleScript;
	if (generateBuildScriptInternal(_engineModuleInfoMap, "Engine", engineModuleScript) == false)
	{
		return false;
	}

	PString userModuleScript;
	if (generateBuildScriptInternal(_userModuleInfoMap, getUserProjectName(), userModuleScript) == false)
	{
		return false;
	}


	PString templateScript;
	if (HFileHelper::ReadAllText(getBuildScriptTemplatePath(), &templateScript) == false)
	{
		return false;
	}

	// 템플릿(BuildTemplate.lua)이 쓰는 값. 엔진 솔루션은 ENGINE_ROOT 가 "" 라 경로 문자열이 예전과 같다.
	PString buildScript;
	buildScript.AppendLine(PString::Format("ENGINE_ROOT = \"%s\"", getEngineRootPrefix()));
	if (HFileHelper::IsProjectMode() == true)
	{
		buildScript.AppendLine(PString::Format("WORKSPACE_NAME = \"%s\"", HFileHelper::ProjectName()));
		buildScript.AppendLine("START_PROJECT = \"JGLauncher\"");
	}
	else
	{
		buildScript.AppendLine("WORKSPACE_NAME = \"JGEngine\"");
	}
	buildScript.Append(templateScript);
	buildScript.AppendLine("");
	buildScript.AppendLine(engineModuleScript);
	buildScript.AppendLine(userModuleScript);

	PString resultPath;
	if (HFileHelper::IsProjectMode() == true)
	{
		resultPath = getGameProjectScriptPath();
	}
	else
	{
		HFileHelper::CombinePath(BuildToolDirectory(), SCRIPT_NAME, &resultPath);
	}

	if (HFileHelper::WriteAllText(resultPath, buildScript) == false)
	{
		return false;
	}

	JG_LOG(BuildTool, ELogLevel::Trace, "Generate Build Script: %s", resultPath);
	return true;
}

bool PBuildTool::generateBuildScriptInternal(const HHashMap<PString, HList<PModuleInfo>>& moduleInfoMap, const PString& inGroupName, PString& outScript)
{
	const PArguments& arguments = getArguments();

	outScript = PString();
	for (const HPair<PString, HList<PModuleInfo>>& pair : moduleInfoMap)
	{
		outScript.Append("\t\t");

		PString groupName = inGroupName;
		groupName.Append("/").Append(pair.first);

		// GroupName
		outScript.Append("group \"").Append(groupName).AppendLine("\"");

		for (const PModuleInfo& moduleInfo : pair.second)
		{
			JG_LOG(BuildTool, ELogLevel::Trace, "Module: %s: Start Generate Build Script", moduleInfo.ModuleName);
			bool bIsSharedLib = moduleInfo.ModuleFormat == "SharedLib";
			// Project
			outScript.Append("\t\t\t");
			outScript.Append("project \"").Append(moduleInfo.ModuleName).AppendLine("\"");
			outScript.Append("\t\t\t\t");

			const PString enginePrefix = getEngineRootPrefix();
			const PString modulePrefix = getModuleRootPrefix(moduleInfo);

			PString thirdPartyPath = enginePrefix + toEngineRelativePath(arguments.ThirdPartyDirectory);

			PString includeDirs;
			includeDirs.Append("\"").Append(modulePrefix).Append(moduleInfo.ModulePath).Append("\", ");
			includeDirs.Append("\"").Append(thirdPartyPath).Append("\", ");
			includeDirs.Append("\"").Append(enginePrefix).Append("Source/").Append("\", ");

			if (bIsSharedLib)
			{
				includeDirs.Append("\"").Append(modulePrefix).Append(moduleInfo.CodeGenPath).Append("\", ");
			}

			PString links;
			PString defines;

// 			PString postcommands = PString::Format(R"(
// 				postbuildcommands {
// 					"copy $(TargetDir)%s.dll $(TargetDir)..\\",
// 					"copy $(TargetDir)%s.lib $(TargetDir)..\\",
// 					"copy $(TargetDir)%s.exp $(TargetDir)..\\",
// 					"copy $(TargetDir)%s.pdb $(TargetDir)..\\",
// 					"copy $(TargetDir)%s.dll $(TargetDir)..\\%s_Dynamic.dll"
// })"
// , moduleInfo.ModuleName, moduleInfo.ModuleName, moduleInfo.ModuleName,
// moduleInfo.ModuleName, moduleInfo.ModuleName, moduleInfo.ModuleName);

	

			_codeGenableModuleSet.insert(moduleInfo.ModuleName);
			for (const PString& moduleName : moduleInfo.ModuleDependencies)
			{
				if (_moduleInfoPool.find(moduleName) == _moduleInfoPool.end())
				{
					JG_LOG(BuildTool, ELogLevel::Error, "Module: %s: is not exists dependency module(%s)", moduleInfo.ModuleName, moduleName);
					continue;
				}

				const PModuleInfo& dependencyModule = _moduleInfoPool[moduleName];
				const PString dependencyPrefix = getModuleRootPrefix(dependencyModule);

				includeDirs.Append("\"").Append(dependencyPrefix).Append(dependencyModule.ModulePath).Append("\", ");
				links.Append("\"").Append(moduleName).Append("\", ");

				if (dependencyModule.ModuleFormat == "SharedLib")
				{
					includeDirs.Append("\"").Append(dependencyPrefix).Append(dependencyModule.CodeGenPath).Append("\", ");
				}
			}

			defines.Append("{");
			for (const PString& define : moduleInfo.Defines)
			{
				defines.Append("\"").Append(define).Append("\", ");
			}
			defines.Append("}");

			outScript.Append("includedirs{ ");
			outScript.Append(includeDirs);
			outScript.AppendLine("}");
			outScript.Append("\t\t\t\t");

			outScript.Append("links{ ");
			outScript.Append(links);
			outScript.AppendLine("}");
			outScript.Append("\t\t\t\t");

			if (bIsSharedLib)
			{
				outScript.Append("SetDynamicCPPProjectConfig(");
			}
			else
			{
				outScript.Append("SetCPPProjectConfig(");
			}

			outScript.Append("\"").Append(moduleInfo.ModuleFormat).Append("\", ");
			outScript.Append("\"").Append(modulePrefix).Append(moduleInfo.ModulePath).Append("\", ");

			if (defines.Empty() == false)
			{
				outScript.Append(defines);
			}

			if (bIsSharedLib)
			{
				outScript.Append(PString(", \"") + modulePrefix + moduleInfo.CodeGenPath + "\"");
			}
			
			outScript.AppendLine(")");
			outScript.Append("\t\t\t\t");

			outScript.AppendLine("filter \"configurations:DevelopEngine\"");
			outScript.Append("\t\t\t\t\t").Append(moduleInfo.ModuleFilters[(int32)EModuleFilter::DevelopEngine].Config).AppendLine("()");
			outScript.Append("\t\t\t\t\t").AppendLine(getDefines(moduleInfo, EModuleFilter::DevelopEngine));
			// if (bIsSharedLib)
			// {
			// 	outScript.Append("\t\t\t\t\t").AppendLine(postcommands);
			// }
			outScript.Append("\t\t\t\t");

			outScript.AppendLine("filter \"configurations:DevelopGame\"");
			outScript.Append("\t\t\t\t\t").Append(moduleInfo.ModuleFilters[(int32)EModuleFilter::DevelopGame].Config).AppendLine("()");
			outScript.Append("\t\t\t\t\t").AppendLine(getDefines(moduleInfo, EModuleFilter::DevelopGame));
			// if (bIsSharedLib)
			// {
			// 	outScript.Append("\t\t\t\t\t").AppendLine(postcommands);
			// }
			outScript.Append("\t\t\t\t");

			outScript.AppendLine("filter \"configurations:ConfirmGame\"");
			outScript.Append("\t\t\t\t\t").Append(moduleInfo.ModuleFilters[(int32)EModuleFilter::ConfirmGame].Config).AppendLine("()");
			outScript.Append("\t\t\t\t\t").AppendLine(getDefines(moduleInfo, EModuleFilter::ConfirmGame));
			// if (bIsSharedLib)
			// {
			// 	outScript.Append("\t\t\t\t\t").AppendLine(postcommands);
			// }
			outScript.Append("\t\t\t\t");

			outScript.AppendLine("filter \"configurations:ReleaseGame\"");
			outScript.Append("\t\t\t\t\t").Append(moduleInfo.ModuleFilters[(int32)EModuleFilter::ReleaseGame].Config).AppendLine("()");
			outScript.Append("\t\t\t\t\t").AppendLine(getDefines(moduleInfo, EModuleFilter::ReleaseGame));
			// if (bIsSharedLib)
			// {
			// 	outScript.Append("\t\t\t\t\t").AppendLine(postcommands);
			// }

			outScript.AppendLine("");
			outScript.AppendLine("");

			JG_LOG(BuildTool, ELogLevel::Trace, "Module: %s: Compelete Generate Build Script", moduleInfo.ModuleName);
		}
	}

	return true;
}

void PBuildTool::insertIncludePCHHeaderCode()
{
	insertIncludePCHHeaderCodeInternal(_engineModuleInfoMap);
	insertIncludePCHHeaderCodeInternal(_userModuleInfoMap);
}

void PBuildTool::insertIncludePCHHeaderCodeInternal(const HHashMap<PString, HList<PModuleInfo>>& moduleInfoMap)
{
	for (const HPair<PString, HList<PModuleInfo>>& pair : moduleInfoMap)
	{
		for (const PModuleInfo& moduleInfo : pair.second)
		{
			// 게임 프로젝트 솔루션을 만들 때는 엔진 소스를 건드리지 않는다 (엔진 쪽은 엔진 PreBuild 가 맡는다).
			if (HFileHelper::IsProjectMode() == true && moduleInfo.bEngineModule == true)
			{
				continue;
			}

			const PString& rootDirectory = moduleInfo.bEngineModule ? HFileHelper::EngineDirectory() : HFileHelper::ProjectDirectory();
			PString modulePath = rootDirectory / moduleInfo.ModulePath;

			HList<PString> cppFileList;

			HFileHelper::FileListInDirectory(modulePath, &cppFileList, true, { ".cpp" });

			for (const PString& cppFilePath : cppFileList)
			{
				PString cppText;
				if (HFileHelper::ReadAllText(cppFilePath, &cppText) == false)
				{
					JG_LOG(BuildTool, ELogLevel::Error, "Module: %s: Fail Insert Include PCH.H Code", moduleInfo.ModuleName);
					continue;
				}

				uint64 pos = cppText.Find("#include \"PCH/PCH.h\"");
				if (pos != PString::NPOS)
				{
					// 이미 있으면 쓰지 않는다. 다시 쓰면 mtime 이 바뀌어 매 생성마다 전체 재컴파일이 된다.
					continue;
				}

				cppText.Insert("#include \"PCH/PCH.h\"\n", 0);
				if (HFileHelper::WriteAllText(cppFilePath, cppText) == false)
				{
					JG_LOG(BuildTool, ELogLevel::Error, "Module: %s: Fail Insert Include PCH.H Code", moduleInfo.ModuleName);
					continue;
				}
			}
		}
	}
}

bool PBuildTool::makeProjectFiles()
{
	PString newScriptPath;
	HFileHelper::CombinePath(BuildToolDirectory(), SCRIPT_NAME, &newScriptPath);

	PString oldScriptPath;
	HFileHelper::CombinePath(HFileHelper::EngineDirectory(), SCRIPT_NAME, &oldScriptPath);


	PString newScriptText;
	if (HFileHelper::ReadAllText(newScriptPath, &newScriptText) == false)
	{
		JG_LOG(BuildTool, ELogLevel::Error, "Fail Read New Script");
		return false;
	}

	if (HFileHelper::WriteAllText(oldScriptPath, newScriptText) == false)
	{
		JG_LOG(BuildTool, ELogLevel::Error, "Fail Replace New Script");
		return false;
	}

	PString premakeFileOriginPath;
	HFileHelper::CombinePath(HFileHelper::EngineBuildDirectory(), PREMAKE_FILE_NAME, &premakeFileOriginPath);
	HFileHelper::AbsolutePath(premakeFileOriginPath, &premakeFileOriginPath);

	PString premakeTempFilePath;
	HFileHelper::CombinePath(HFileHelper::EngineDirectory(), PREMAKE_FILE_NAME, &premakeTempFilePath);
	HFileHelper::AbsolutePath(premakeTempFilePath, &premakeTempFilePath);

	if (HFileHelper::CopyFileOrDirectory(premakeFileOriginPath, premakeTempFilePath) == false)
	{
		JG_LOG(BuildTool, ELogLevel::Error, "Fail Copy Premake File");
		return false;
	}

	PString batCommand;
	batCommand.Append("call ").Append("\"").Append(premakeTempFilePath).Append("\"").Append(" vs2022 --file=").Append(oldScriptPath);

	PString batFilePath;
	HFileHelper::CombinePath(HFileHelper::EngineDirectory(), BATCH_NAME, &batFilePath);
	HFileHelper::AbsolutePath(batFilePath, &batFilePath);

	if (HFileHelper::WriteAllText(batFilePath, batCommand) == false)
	{
		JG_LOG(BuildTool, ELogLevel::Error, "Fail Create Batch File");
		return false;
	}
	
	system(batFilePath.GetCStr());

	//HFileHelper::RemoveFileOrDirectory(batFilePath);
	//HFileHelper::RemoveFileOrDirectory(oldScriptPath);
	//HFileHelper::RemoveFileOrDirectory(premakeTempFilePath);

	return true;
}

bool PBuildTool::makeGameProjectFiles()
{
	// 스크립트는 generateBuildScript 가 프로젝트 루트에 썼다. premake 는 엔진 것을 그대로 쓰고 엔진 트리에는 아무것도 쓰지 않는다.
	PString premakePath;
	HFileHelper::CombinePath(HFileHelper::EngineBuildDirectory(), PREMAKE_FILE_NAME, &premakePath);
	HFileHelper::AbsolutePath(premakePath, &premakePath);

	const PString scriptPath = getGameProjectScriptPath();

	// cmd /c 는 명령 전체가 따옴표로 시작하면 바깥 따옴표를 벗기므로 한 겹 더 감싼다.
	PString command;
	command.Append("\"\"").Append(premakePath).Append("\" vs2022 --file=\"").Append(scriptPath).Append("\"\"");

	JG_LOG(BuildTool, ELogLevel::Trace, "Run: %s", command);
	int32 exitCode = system(command.GetCStr());
	if (exitCode != 0)
	{
		JG_LOG(BuildTool, ELogLevel::Error, "premake failed (%d): %s", exitCode, scriptPath);
		return false;
	}

	return true;
}

bool PBuildTool::copyThirdPartyBinaries() const
{
	// 모든 모듈이 PCH.h 의 #pragma comment(lib) 로 zlibstatic.lib · assimp-mt.lib 를 링크하고(libdirs = Bin/<Config>),
	// Graphics.dll 은 실행 시 assimp-vc143-mt.dll 을 찾는다. assimp-mt.lib 가 가리키는 DLL 이름이 assimp-vc143-mt.dll 이라
	// ThirdParty 의 assimp-mt.dll(같은 파일)을 그 이름으로 복사한다. 엔진 Bin 에는 손으로 복사돼 있던 것들이다.
	struct HThirdPartyFile
	{
		const char* Source;
		const char* Target;
	};

	static const HThirdPartyFile files[] =
	{
		{ "ThirdParty/zlib/zlibstatic.lib",  "zlibstatic.lib" },
		{ "ThirdParty/assimp/assimp-mt.lib", "assimp-mt.lib" },
		{ "ThirdParty/assimp/assimp-mt.dll", "assimp-vc143-mt.dll" },
	};

	static const char* configurations[] = { "DevelopEngine", "DevelopGame", "ConfirmGame", "ReleaseGame" };

	bool bResult = true;
	for (const char* configuration : configurations)
	{
		PString binDirectory;
		HFileHelper::CombinePath(HFileHelper::ProjectBinDirectory(), configuration, &binDirectory);
		std::error_code errorCode;
		fs::create_directories(binDirectory.GetRawString(), errorCode);

		for (const HThirdPartyFile& file : files)
		{
			PString sourcePath;
			HFileHelper::CombinePath(HFileHelper::EngineDirectory(), file.Source, &sourcePath);

			PString targetPath;
			HFileHelper::CombinePath(binDirectory, file.Target, &targetPath);

			// 실행 중인 게임이 DLL 을 잡고 있어도 실패하지 않도록 바뀐 파일만 덮어쓴다.
			fs::copy_file(sourcePath.GetRawString(), targetPath.GetRawString(), fs::copy_options::update_existing, errorCode);
			if (errorCode.value() != 0)
			{
				JG_LOG(BuildTool, ELogLevel::Error, "Fail copy %s -> %s: %s", sourcePath, targetPath, errorCode.message().c_str());
				bResult = false;
			}
		}
	}

	return bResult;
}

bool PBuildTool::findModuleInfo(const PString& modulePath, PModuleInfo* outModuleInfo) const
{
	if (outModuleInfo == nullptr)
	{
		return false;
	}

	const PArguments& arguments = getArguments();

	HList<PString> fileListInModule;
	HFileHelper::FileListInDirectory(modulePath, &fileListInModule);

	PString moduleName;
	HFileHelper::FileName(modulePath, &moduleName);

	for (const PString& filePath : fileListInModule)
	{
		if (HFileHelper::IsDirectory(filePath) == true)
		{
			continue;
		}

		PString fileName;
		HFileHelper::FileName(filePath, &fileName);

		HList<PString> split = fileName.Split('.');
		if (split.empty() == false && split.size() == 3)
		{
			PString fileExtension         = split[2];
			PString moduleRecognitionName = split[1];

			if (fileExtension != arguments.ModuleInfoFileExtension ||
				moduleRecognitionName != arguments.ModuleRecognitionName)
			{
				continue;
			}

			if (moduleName != split[0])
			{
				continue;
			}

			PString jsonText;
			if (HFileHelper::ReadAllText(filePath, &jsonText) == false)
			{
				JG_LOG(BuildTool, ELogLevel::Error, "module(%s) fail read json file", moduleName);
				continue;
			}

			PJson json;
			if (PJson::ToObject(jsonText, &json) == false)
			{
				JG_LOG(BuildTool, ELogLevel::Error, "module(%s) fail PJson::ToObject", moduleName);
				continue;
			}

			if (json.GetData("ModuleInfo", outModuleInfo) == false)
			{
				JG_LOG(BuildTool, ELogLevel::Error, "module(%s) fail json getdata", moduleName);
				continue;
			}

			return true;
		}
	}

	return false;
}

PString PBuildTool::getDefines(const PModuleInfo& moduleInfo, EModuleFilter filter) const
{
	const PArguments& args = getArguments();

	PString result;
	result.Append("defines{");

	for (const PString& define : args.Defines)
	{
		result.Append("\"").Append(define).Append("\", ");
	}

	for (const PString& define : args.GlobalFilters[(int32)filter].Defines)
	{
		result.Append("\"").Append(define).Append("\", ");
	}
	for (const PString& define : moduleInfo.ModuleFilters[(int32)filter].Defines)
	{
		result.Append("\"").Append(define).Append("\", ");
	}
	result.Append("}");

	return result;
}

PString PBuildTool::getUserProjectName() const
{
	if (HFileHelper::IsProjectMode() == true)
	{
		return HFileHelper::ProjectName();
	}

	const PArguments& arguments = getArguments();

	PString UserProjectName;
	HFileHelper::FileName(arguments.UserWorkDirectory, &UserProjectName);

	return UserProjectName;
}

const PString& PBuildTool::getBuildScriptTemplatePath() const
{
	const PArguments& arguments = getArguments();
	return arguments.BuildScriptTemplatePath;
}

PString PBuildTool::getGameProjectScriptPath() const
{
	return HFileHelper::ProjectDirectory() + HFileHelper::ProjectName() + ".lua";
}

PString PBuildTool::getEngineRootPrefix() const
{
	if (HFileHelper::IsProjectMode() == false)
	{
		return PString();
	}

	PString engineDirectory;
	HFileHelper::AbsoluteDirectory(HFileHelper::EngineDirectory(), &engineDirectory);
	return engineDirectory;
}

PString PBuildTool::getModuleRootPrefix(const PModuleInfo& moduleInfo) const
{
	if (moduleInfo.bEngineModule == true)
	{
		return getEngineRootPrefix();
	}

	// 게임 모듈 경로는 프로젝트 루트 기준이고 스크립트도 프로젝트 루트에 있다.
	return PString();
}

PString PBuildTool::toEngineRelativePath(const PString& path) const
{
	PString normalizedPath;
	HFileHelper::AbsoluteDirectory(path, &normalizedPath);

	PString engineDirectory;
	HFileHelper::AbsoluteDirectory(HFileHelper::EngineDirectory(), &engineDirectory);

	if (PString::ToLower(normalizedPath).StartWidth(PString::ToLower(engineDirectory)) == false)
	{
		return path;
	}

	normalizedPath.Remove(0, engineDirectory.Length());
	if (normalizedPath.EndWidth("/") == true)
	{
		normalizedPath.PopBack();
	}

	return normalizedPath;
}
