#pragma once
#include "Core.h"
#include "Arguments.h"


struct HHeaderInfo;

struct HMeta
{
	HHashMap<PString, HHashSet<PString>> Metas;

public:
	void AddMeta(const PString& key, const PString& value)
	{
		Metas[key].insert(value);
	}
};

struct HProperty
{
	PString Type;
	PString Name;
	HMeta MetaData;
};

struct HFunction
{
	HList<HProperty> Argmuments;
	PString Return;
	PString Name;
	HMeta MetaData;
};

struct HClass
{
	PString Name;
	HList<PString>   BaseClasses;
	HList<HProperty> Properties;
	HList<HFunction> Functions;
	HMeta MetaData;

	HHeaderInfo* OwnerHeaderInfo = nullptr;

	HClass(HHeaderInfo* pOwnerHeaderInfo) : OwnerHeaderInfo(pOwnerHeaderInfo) {}

	void GetCodeGenStaticCreateFuncName(PString* outName) const;
	void GetCodeGenCreateFuncName(PString* outName) const;
	void GetCodeGenCreateObjectFuncName(PString* outName) const;
};

struct HEnumElement
{
	HMeta MetaData;
	PString Name;
	int32 Value;
};

struct HEnum
{
	enum 
	{
		AUTO_REDIRECT = 0
	};

	PString Name;
	HList<HEnumElement> Elements;
	HMeta MetaData;

	int32 LastEnumValue = 0;
	HHeaderInfo* OwnerHeaderInfo = nullptr;

	HEnum(HHeaderInfo* pOwnerHeaderInfo) : OwnerHeaderInfo(pOwnerHeaderInfo) {}

	void GetCodeGenStaticCreateFuncName(PString* outName) const;
};

struct HHeaderInfo
{
	PString ModuleName;
	PString FileName;
	PString Content;
	PString LocalRelativePath;

	HList<HClass> Classes;
	HList<HEnum> Enums;
};

class PHeaderTool : public IMemoryObject
{
public:
	static constexpr char const* ARGUMENTS_JSON_FILE_NAME = "headertool_arguments.json";

private:
	PArguments _args;
	HList<HHeaderInfo> _engineHeaderInfos;
	HList<HHeaderInfo> _userHeaderInfos;

	HHashSet<PString> _engineModuleSet;
	HHashSet<PString> _userModuleSet;

public:
	static const PString& HeaderToolDirectory();

public:
	PHeaderTool(const PArguments& args);
	virtual ~PHeaderTool() = default;

public:
	bool Run();

private:
	// 카테고리 아래에서 <폴더명>.module.json 이 SharedLib 인 모듈을 모은다. 코드젠 대상이 이 집합이다.
	void collectSharedLibModules(const PString& workDirectory, const HHashSet<PString>& workCategories, HHashSet<PString>* outModuleSet) const;

	bool collectionHeaderFiles();
	bool collectionHeaderFiles_Internal(const PString& inDir, const HHashSet<PString>& allowedModules, HList<HHeaderInfo>& inHeaderInfos);

	bool extractReflectionDatas();
	bool extractReflectionDatasInternal(HList<HHeaderInfo>& inHeaderInfos);

	bool isCanAnalysisClass(const PString& line) const;
	bool isCanAnalysisProperty(const PString& line) const;
	bool isCanAnalysisFunction(const PString& line) const;
	bool isCanAnalysisEnum(const PString& line) const;
	bool isCanAnalysisEnumMeta(const PString& line) const;

	bool analysisBaseClass(const PString& line, HClass* outClass);
	bool analysisProperty(const PString& line, HProperty* outProperty);
	bool analysisFunction(const PString& line, HFunction* outFunction);
	bool analysisMetaDatas(const PString& line, const PString& token, HMeta* pMeta);
	bool analysisEnum(const PString& line, HEnum* pEnum);
	bool analysisEnumElement(const PString& line, HEnum* pEnum);

	bool generateCodeGenFiles();
	bool generateCodeGenFilesInternal(const PString& codeGenRootDirectory, const HList<HHeaderInfo>& headerInfos, const HHashSet<PString>& moduleSet);
	bool generateCodeGenHeaderSourceCode(const HHeaderInfo& headerInfo, PString* outCode);
	bool generateCodeGenCPPSoucreCode(const HHeaderInfo& headerInfo, PString* outCode);
	bool generateCodeGenRegistration(const PString& inModuleName, HQueue<const HClass*>& collectedClassQueue, HQueue<const HEnum*>& collectedEnumQueue, PString* outCode);
	
	const PArguments& getArguments() const;
};