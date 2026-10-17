// Memory_TODO 2-5 검토용 크기 프로브 (2026-09-29). 엔진 헤더로 컴파일해 오류 메시지의 배열 크기로 sizeof 를 읽는다(일부러 실패시킨다).
// 실행 (cmd, VS 2022 Professional 기준):
//   call "C:\Program Files\Microsoft Visual Studio\2022\Professional\VC\Auxiliary\Build\vcvars64.bat"
//   set INC=/I"C:\JG\JGEngine\Source\Runtime\Core" /I"C:\JG\JGEngine\Source\ThirdParty" /I"C:\JG\JGEngine\Source"
//   디버그 : cl /nologo /c /std:c++latest /utf-8 /EHsc /MDd /D_DEBUG /D_CORE /D_PLATFORM_WINDOWS /D_DIRECTX12 /D_JGPROJECT /D_DEVELOPENGINE %INC% sizeprobe_2-5.cpp
//   릴리스 : cl /nologo /c /std:c++latest /utf-8 /EHsc /MD /DNDEBUG /D_CORE /D_PLATFORM_WINDOWS /D_DIRECTX12 /D_JGPROJECT /D_RELEASEGAME %INC% sizeprobe_2-5.cpp
// 읽는 법: "sizeprobe_2-5.cpp(N): error C2440: ... 'char (*)[크기]'" 의 N 이 아래 줄 번호, 크기가 sizeof. L15/L16 은 (bool 값 + 1).
// 2026-09-29 측정 (디버그 / 릴리스):
//   PString 56/48, 기반 없음 48/40 | PName 72/32, 기반 없음 64/24, ID만(7-1) 48/8 | JGType 96/56, 기반 없음 80/40
//   PSharedPtr 32/32, 기반 없음 24/24 | PWeakPtr 32/32 | PJsonData 48/48 | PDateTime 24/24 | PTimespan 24/24 | IMemoryObject만 8/8
//   is_trivially_copyable<PName> = false, is_polymorphic<PSharedPtr> = true
// 2-5 적용 뒤 다시 돌려 "기반 없음" 값과 같아졌는지 확인한다 (그때는 거울 구조체 줄을 지워도 된다).
#include "PCH/PCH.h"
#include "String/String.h"
#include "String/Name.h"
#include "Object/JGType.h"
#include "Memory/Memory.h"
#include "FileIO/Json.h"
#include "Misc/DateTime.h"
#include "Misc/Timespan.h"

struct HProbeObject : public IMemoryObject {};

// IMemoryObject 를 빼면 남는 멤버만 옮긴 거울 구조체
struct HProbePStringNoBase { uint64 Code; HRawString Raw; };
struct HProbePNameNoBase
{
	uint64 Id;
	std::weak_ptr<HAtomicInt32> Weak;
#ifdef _DEBUG
	HRawString Str;
#endif
};
struct HProbePNameIdOnly
{
	uint64 Id;
#ifdef _DEBUG
	HRawString Str;
#endif
};
struct HProbeJGTypeNoBase { uint64 ID; uint64 Size; HProbePNameNoBase Name; };
struct HProbeSharedPtrNoBase { void* Ptr; void* Ref; void* Weak; };

char (*L01_PString)[sizeof(PString)] = 1;
char (*L02_PString_nobase)[sizeof(HProbePStringNoBase)] = 1;
char (*L03_PName)[sizeof(PName)] = 1;
char (*L04_PName_nobase)[sizeof(HProbePNameNoBase)] = 1;
char (*L05_PName_idonly_nobase)[sizeof(HProbePNameIdOnly)] = 1;
char (*L06_JGType)[sizeof(JGType)] = 1;
char (*L07_JGType_nobase)[sizeof(HProbeJGTypeNoBase)] = 1;
char (*L08_PSharedPtr)[sizeof(PSharedPtr<HProbeObject>)] = 1;
char (*L09_PSharedPtr_nobase)[sizeof(HProbeSharedPtrNoBase)] = 1;
char (*L10_PWeakPtr)[sizeof(PWeakPtr<HProbeObject>)] = 1;
char (*L11_PJsonData)[sizeof(PJsonData)] = 1;
char (*L12_PDateTime)[sizeof(PDateTime)] = 1;
char (*L13_PTimespan)[sizeof(PTimespan)] = 1;
char (*L14_IMemoryObject_only)[sizeof(HProbeObject)] = 1;
char (*L15_PName_trivially_copyable)[std::is_trivially_copyable<PName>::value + 1] = 1;
char (*L16_PSharedPtr_polymorphic)[std::is_polymorphic<PSharedPtr<HProbeObject>>::value + 1] = 1;
