#pragma once
#include "CoreDefines.h"
#include "FileIO/Jsonable.h"
#include "Memory/Allocator.h"
#include "String/String.h"

// 게임 창 설정. 패키지 서술자(Bin/JGPackage.json)와 .jgproject 의 "Package" 섹션이 같이 쓴다.
// 기본 1920×1080 창 모드(사용자 지시 2026-10-02). 모니터 작업 영역보다 크면 호스트가 16:9 로 줄인다.
class HGameWindowSettings : public IJsonable
{
public:
	static constexpr const char* WindowedMode             = "Windowed";
	static constexpr const char* BorderlessFullscreenMode = "BorderlessFullscreen";

	PString Title;                  // 비어 있으면 프로젝트(패키지) 이름
	int32   Width  = 1920;
	int32   Height = 1080;
	PString Mode   = WindowedMode;  // Windowed / BorderlessFullscreen
	bool    bVSync = true;

public:
	virtual ~HGameWindowSettings() = default;

protected:
	virtual void WriteJson(PJsonData& json) const override;
	virtual void ReadJson(const PJsonData& json) override;
};

// 게임 패키지(쿠킹 결과)의 서술자 <패키지>/Bin/JGPackage.json. 쿠킹 명령(JGConsole -runCook)이 쓰고,
// 패키지 실행(JGGameLaunch → GCoreSystem::Create 의 PackageDirectory)이 읽는다(DevOps GP-D1, 2026-10-02).
//
// {
//   "Package": {
//     "Name": "MyGame", "Version": "0.1.0", "BuildConfig": "ReleaseGame",
//     "EngineCommit": "0bd6356", "CookedAt": "2026-10-02T16:20:00+09:00",
//     "LaunchModule": "GameHost",
//     "RuntimeModules": [ "Graphics", "GameFrameWork", "GameHost", "MyGame" ],
//     "GameModules": [ "MyGame" ],
//     "Window": { "Title": "MyGame", "Width": 1920, "Height": 1080, "Mode": "Windowed", "VSync": true }
//   }
// }
class HPackageDescriptor : public IJsonable
{
public:
	static constexpr const char* BinDirectoryName = "Bin";
	static constexpr const char* FileName         = "JGPackage.json";

	PString        Name;
	PString        Version;
	PString        BuildConfig;
	PString        EngineCommit;
	PString        CookedAt;
	PString        LaunchModule = "GameHost";
	HList<PString> RuntimeModules;  // codeGen 이 시작할 때 올리는 모듈(의존 순서). 패키지 Bin 에 있는 DLL 전부
	HList<PString> GameModules;     // 호스트가 연결하는 게임 모듈(.jgproject GameModules)
	HGameWindowSettings Window;

public:
	virtual ~HPackageDescriptor() = default;

	// <packageDirectory>/Bin/JGPackage.json 을 읽는다. 없거나 Name 이 비면 false.
	static bool Load(const PString& packageDirectory, HPackageDescriptor* outDescriptor);
	// <packageDirectory>/Bin/JGPackage.json 에 쓴다(쿠킹 명령).
	bool Save(const PString& packageDirectory) const;

	static void FilePath(const PString& packageDirectory, PString* outPath);

protected:
	virtual void WriteJson(PJsonData& json) const override;
	virtual void ReadJson(const PJsonData& json) override;
};
