#pragma once
#include "Core.h"
#include "Arguments.h"

using EModuleFilter = EGlobalFilter;
class HModuleFilter : public IJsonable
{
public:
	PString Config;
	HList<PString> Defines;

public:
	virtual ~HModuleFilter() = default;

protected:
	virtual void WriteJson(PJsonData& json) const override
	{
		json.AddMember("Config", Config);
		json.AddMember("Defines", Defines);
	}

	virtual void ReadJson(const PJsonData& json) override
	{
		if (json.GetData("Config", &Config) == false)
		{
			JG_LOG(BuildTool, ELogLevel::Error, "Defines: fail read json data");
		}

		if (json.GetData("Defines", &Defines) == false)
		{
			JG_LOG(BuildTool, ELogLevel::Error, "Defines: fail read json data");
		}
	}
};

class PModuleInfo : public IMemoryObject, public IJsonable
{
public:
	PString ModuleFormat;

	HList<PString> ModuleDependencies;

	PString ModuleName;
	PString ModulePath;
	PString CodeGenPath;
	HList<PString> Defines;

	HModuleFilter ModuleFilters[(int32)EModuleFilter::Count];

	// json 에 없는 실행 중 정보. 엔진 모듈이면 true, 게임 프로젝트 모듈이면 false.
	// ModulePath · CodeGenPath 는 자기 루트 기준이라, 게임 프로젝트 솔루션에서는 엔진 모듈 경로에 엔진 루트를 붙여야 한다.
	bool bEngineModule = true;

	// 선택 필드 "Standalone": true — 엔진과 무관한 단독 실행 파일(게임 패키지 부트스트랩 JGGameBootstrap).
	// PCH 를 넣지 않고(PCH 의 #pragma comment(lib) 도 없음) Core 를 링크하지 않으며 CRT 를 정적으로 링크한다(DevOps GP-D2, 2026-10-02).
	// 템플릿(ModuleInfoTemplate.json)에는 쓰지 않는다 — 없으면 false.
	bool bStandalone = false;

	//PString DevelopEngineFilter;
	//PString DevelopGameFilter;
	//PString DevelopConfirmGameFilter;
	//PString DevelopReleaseGameFilter;

	virtual ~PModuleInfo() = default;
protected:
	virtual void WriteJson(PJsonData& json) const
	{
		json.AddMember("ModuleFormat", ModuleFormat);
		json.AddMember("ModuleDependencies", ModuleDependencies);
		json.AddMember("ModuleName", ModuleName);
		json.AddMember("ModulePath", ModulePath);
		json.AddMember("Defines", Defines);


		json.AddMember("DevelopEngine", ModuleFilters[(int32)EModuleFilter::DevelopEngine]);
		json.AddMember("DevelopGame", ModuleFilters[(int32)EModuleFilter::DevelopGame]);
		json.AddMember("ConfirmGame", ModuleFilters[(int32)EModuleFilter::ConfirmGame]);
		json.AddMember("ReleaseGame", ModuleFilters[(int32)EModuleFilter::ReleaseGame]);
	}

	virtual void ReadJson(const PJsonData& json)
	{
		if (json.GetData("ModuleFormat", &ModuleFormat) == false)
		{
			JG_LOG(BuildTool, ELogLevel::Error, "ModuleFormat: fail read json data");
		}

		if (json.GetData("ModuleDependencies", &ModuleDependencies) == false)
		{
			JG_LOG(BuildTool, ELogLevel::Error, "ModuleDependencies: fail read json data");
		}

		if (json.GetData("ModuleName", &ModuleName) == false)
		{
			JG_LOG(BuildTool, ELogLevel::Error, "ModuleName: fail read json data");
		}

		if (json.GetData("ModulePath", &ModulePath) == false)
		{
			JG_LOG(BuildTool, ELogLevel::Error, "ModulePath: fail read json data");
		}

		if (json.GetData("Defines", &Defines) == false)
		{
			JG_LOG(BuildTool, ELogLevel::Error, "Defines: fail read json data");
		}

		if (json.GetData("DevelopEngine", &ModuleFilters[(int32)EModuleFilter::DevelopEngine]) == false)
		{
			JG_LOG(BuildTool, ELogLevel::Error, "DevelopEngine: fail read json data");
		}

		if (json.GetData("DevelopGame", &ModuleFilters[(int32)EModuleFilter::DevelopGame]) == false)
		{
			JG_LOG(BuildTool, ELogLevel::Error, "DevelopGame: fail read json data");
		}

		if (json.GetData("ConfirmGame", &ModuleFilters[(int32)EModuleFilter::ConfirmGame]) == false)
		{
			JG_LOG(BuildTool, ELogLevel::Error, "ConfirmGame: fail read json data");
		}

		if (json.GetData("ReleaseGame", &ModuleFilters[(int32)EModuleFilter::ReleaseGame]) == false)
		{
			JG_LOG(BuildTool, ELogLevel::Error, "ReleaseGame: fail read json data");
		}

		// 선택 필드라 없어도 오류가 아니다.
		json.GetData("Standalone", &bStandalone);

		HFileHelper::CombinePath(HFileHelper::EngineCodeGenDirectory(), ModuleName + "/", &CodeGenPath);
		CodeGenPath.Remove(0, HFileHelper::EngineDirectory().Length());
	}
};