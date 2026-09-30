#pragma once
#include "Core.h"
#include "Arguments.h"
#include "ModuleInfo.h"

class PBuildTool : public IMemoryObject
{
public:
	static constexpr char const* ARGUMENTS_JSON_FILE_NAME = "buildtool_arguments.json";
	static constexpr char const* SCRIPT_NAME = "jgengine.lua";
	static constexpr char const* BATCH_NAME = "jgengine.bat";
	static constexpr char const* PREMAKE_FILE_NAME = "premake5.exe";
	static const PString& BuildToolDirectory();
private:
	PArguments _arguments;

	HHashMap<PString, HList<PModuleInfo>> _engineModuleInfoMap;
	HHashMap<PString, HList<PModuleInfo>> _userModuleInfoMap;

	HHashMap<PString, PModuleInfo> _moduleInfoPool;
	HHashSet<PString> _codeGenableModuleSet;

public:
	PBuildTool(const PArguments& args);
	virtual ~PBuildTool() = default;

	bool Run();
private:
	const PArguments& getArguments() const;
	bool collectionModuleInfos(const PString& workDir, const HHashSet<PString>& workCategories, bool bEngineModule, HHashMap<PString, HList<PModuleInfo>>& outModuleInfoMap);
	void collectionModuleInfosInternal(const PString& categoryName, const PString& inCategoryPath, bool bEngineModule, HHashMap<PString, HList<PModuleInfo>>& outModuleInfoMap);
	bool generateBuildScript();
	bool generateBuildScriptInternal(const HHashMap<PString, HList<PModuleInfo>>& moduleInfoMap, const PString& inGroupName, PString& outScript);
	void insertIncludePCHHeaderCode();
	void insertIncludePCHHeaderCodeInternal(const HHashMap<PString, HList<PModuleInfo>>& moduleInfoMap);
	bool makeProjectFiles();
	bool makeGameProjectFiles();
	bool copyThirdPartyBinaries() const;
	bool findModuleInfo(const PString& modulePath, PModuleInfo* outModuleInfo) const;
	PString getDefines(const PModuleInfo& moduleInfo, EModuleFilter filter) const;
	PString getUserProjectName() const;
	const PString& getBuildScriptTemplatePath() const;
	PString getGameProjectScriptPath() const;

	// 생성 스크립트의 경로 앞에 붙일 루트. 엔진 솔루션이면 모두 "" (스크립트가 엔진 루트에 있다).
	// 게임 프로젝트 솔루션이면 스크립트가 프로젝트 루트에 있으므로 엔진 모듈에만 엔진 절대경로를 붙인다.
	PString getEngineRootPrefix() const;
	PString getModuleRootPrefix(const PModuleInfo& moduleInfo) const;
	// 엔진 루트 기준 상대경로("Source/ThirdParty"). 인자 json 의 "../../..." 와 절대경로를 모두 받는다.
	PString toEngineRelativePath(const PString& path) const;
};