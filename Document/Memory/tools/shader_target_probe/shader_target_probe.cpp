// 5-4 검증 (2026-09-29): 엔진 PDX12ShaderCompiler::compile 과 같은 호출(D3DCompile2, D3DCOMPILE_PACK_MATRIX_ROW_MAJOR, 진입점 cs_main)로
// 컴퓨트 셰이더를 옛 타깃 cs_6_0 과 새 타깃 cs_5_1 에 컴파일해 본다. 레지스터 space 는 셰이더 모델 5.1 기능이다.
// 빌드·실행: build.bat <출력 폴더>  ->  <출력 폴더>\shader_target_probe.exe  (기대: cs_6_0 실패, cs_5_1 성공, 종료 코드 0)

#include <windows.h>
#include <d3dcompiler.h>
#include <cstdio>
#include <cstring>

#pragma comment(lib, "d3dcompiler.lib")

namespace
{
	const char* ComputeSource =
		"cbuffer Params : register(b0, space0)\n"
		"{\n"
		"	uint Width;\n"
		"	uint Height;\n"
		"};\n"
		"RWTexture2D<float4> Output : register(u0, space1);\n"
		"[numthreads(8, 8, 1)]\n"
		"void cs_main(uint3 id : SV_DispatchThreadID)\n"
		"{\n"
		"	if (id.x < Width && id.y < Height)\n"
		"	{\n"
		"		Output[id.xy] = float4(id.x / (float)Width, id.y / (float)Height, 0.0f, 1.0f);\n"
		"	}\n"
		"}\n";

	bool compile(const char* inTarget)
	{
		ID3DBlob* byteCode = nullptr;
		ID3DBlob* errors   = nullptr;
		const HRESULT hr = D3DCompile2(ComputeSource, strlen(ComputeSource), nullptr, nullptr, nullptr,
			"cs_main", inTarget, D3DCOMPILE_PACK_MATRIX_ROW_MAJOR, 0, 0, nullptr, 0, &byteCode, &errors);

		printf("%-7s : hr = 0x%08X, bytecode %zu bytes\n", inTarget, (unsigned)hr, byteCode != nullptr ? (size_t)byteCode->GetBufferSize() : (size_t)0);
		if (errors != nullptr)
		{
			printf("          %s\n", (const char*)errors->GetBufferPointer());
			errors->Release();
		}
		if (byteCode != nullptr)
		{
			byteCode->Release();
		}
		return SUCCEEDED(hr);
	}
}

int main()
{
	const bool bOldTarget = compile("cs_6_0");
	const bool bNewTarget = compile("cs_5_1");
	printf("RESULT: cs_6_0 %s, cs_5_1 %s\n", bOldTarget ? "ok" : "FAILED", bNewTarget ? "ok" : "FAILED");
	return (bOldTarget == false && bNewTarget == true) ? 0 : 1;
}
