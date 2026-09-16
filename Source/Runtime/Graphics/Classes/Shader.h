#pragma once
#include "Core.h"
#include "JGGraphicsDefine.h"

// 컴파일된 셰이더의 공통 인터페이스.
class IRawShader
{
public:
	virtual ~IRawShader() = default;
	
	virtual bool IsValid() const = 0;
};


// 그래픽스 셰이더 컴파일 인자.
// Macros에는 머터리얼 도메인 정의(MATERIAL_DOMAIN_SCENE 등)처럼 조합별 전처리 정의가 들어온다.
struct HGraphicsShaderCompileArguments
{
	PString SourceCode;
	EShaderCompileFlags Flags = EShaderCompileFlags::None;
	HList<HPair<PName, PName>> Macros;
};

class IRawGraphicsShader : public IRawShader
{
public:
	virtual ~IRawGraphicsShader() = default;

	// 소스 코드를 컴파일해 바이트코드를 내부에 보관한다.
	// 실패하면 outError(널 허용)에 컴파일러 메시지가 채워지고 false를 반환한다.
	virtual bool Compile(const HGraphicsShaderCompileArguments& inArgs, PString* outError) = 0;
};

class IRawComputeShader : public IRawShader
{
public:
	virtual ~IRawComputeShader() = default;
};