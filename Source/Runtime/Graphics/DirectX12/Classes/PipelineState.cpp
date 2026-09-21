#include "PCH/PCH.h"
#include "PipelineState.h"
#include "JGGraphicsHelper.h"
#include "DirectX12/DirectX12API.h"
#include "DX12Shader.h"
#include "RootSignature.h"

namespace
{
	// 그래픽 PSO 캐시 키.
	// 이전에는 _desc 원본 바이트 전체를 HashState로 해시했다. 두 가지 문제가 있었다.
	//  1. D3D12_DEPTH_STENCIL_DESC 등에는 패딩 바이트가 있다. 매 프레임 스택에 만든 CD3DX12_* 임시 객체를 대입하면 그 패딩(쓰레기 값)이
	//     함께 복사돼 해시가 프레임마다 달라졌고, 프레임마다 새 PSO를 만들어 캐시에 넣었다. D3D 객체와 캐시 노드가 계속 쌓여
	//     32바이트 풀 블록(4096개)이 고갈되면 push_back이 널 포인터에 생성자를 돌려 크래시했다. (2026-09-21)
	//  2. 셰이더 바이트코드는 포인터 값으로만 비교돼 같은 코드라도 주소가 다르면 캐시 미스였다. (5-10)
	// 지금은 필드 단위로 해시하고, 포인터는 가리키는 내용을 해시한다. 루트 서명은 전역 캐시에서 같은 설명이면 같은 객체가 나오므로 포인터 정체성을 쓴다.
	uint64 hashBytes(const void* inData, uint64 inSize, uint64 inHash)
	{
		const uint8* bytes     = (const uint8*)inData;
		const uint64 wordCount = inSize / sizeof(uint32);
		if (wordCount > 0)
		{
			inHash = HHash::HashRange((const uint32*)bytes, (const uint32*)bytes + wordCount, inHash);
		}

		const uint64 tail = inSize - wordCount * sizeof(uint32);
		if (tail > 0)
		{
			uint32 last = 0;
			memcpy(&last, bytes + wordCount * sizeof(uint32), tail);
			inHash = HHash::HashState(&last, 1, inHash);
		}
		return inHash;
	}

	// 4바이트 또는 8바이트 스칼라 하나. (BOOL, UINT, FLOAT, 열거형, 포인터 값)
	template<class T>
	uint64 hashValue(const T& inValue, uint64 inHash)
	{
		static_assert(sizeof(T) == 4 || sizeof(T) == 8, "hashValue expects a 4 or 8 byte scalar");
		return HHash::HashState(&inValue, 1, inHash);
	}

	uint64 hashShader(const D3D12_SHADER_BYTECODE& inCode, uint64 inHash)
	{
		inHash = hashValue((uint64)inCode.BytecodeLength, inHash);
		if (inCode.pShaderBytecode != nullptr && inCode.BytecodeLength > 0)
		{
			inHash = hashBytes(inCode.pShaderBytecode, inCode.BytecodeLength, inHash);
		}
		return inHash;
	}

	uint64 hashStencilOp(const D3D12_DEPTH_STENCILOP_DESC& inOp, uint64 inHash)
	{
		inHash = hashValue(inOp.StencilFailOp, inHash);
		inHash = hashValue(inOp.StencilDepthFailOp, inHash);
		inHash = hashValue(inOp.StencilPassOp, inHash);
		inHash = hashValue(inOp.StencilFunc, inHash);
		return inHash;
	}

	uint64 computeGraphicsPSOHash(const D3D12_GRAPHICS_PIPELINE_STATE_DESC& inDesc, const HList<D3D12_INPUT_ELEMENT_DESC>& inElements)
	{
		uint64 hash = 2166136261U;

		hash = hashValue((uint64)inDesc.pRootSignature, hash);

		hash = hashShader(inDesc.VS, hash);
		hash = hashShader(inDesc.PS, hash);
		hash = hashShader(inDesc.DS, hash);
		hash = hashShader(inDesc.HS, hash);
		hash = hashShader(inDesc.GS, hash);

		// 스트림 출력은 쓰지 않는다. 개수만 넣는다.
		hash = hashValue(inDesc.StreamOutput.NumEntries, hash);
		hash = hashValue(inDesc.StreamOutput.NumStrides, hash);
		hash = hashValue(inDesc.StreamOutput.RasterizedStream, hash);

		hash = hashValue(inDesc.BlendState.AlphaToCoverageEnable, hash);
		hash = hashValue(inDesc.BlendState.IndependentBlendEnable, hash);
		for (uint32 i = 0; i < 8; ++i)
		{
			const D3D12_RENDER_TARGET_BLEND_DESC& blend = inDesc.BlendState.RenderTarget[i];
			hash = hashValue(blend.BlendEnable, hash);
			hash = hashValue(blend.LogicOpEnable, hash);
			hash = hashValue(blend.SrcBlend, hash);
			hash = hashValue(blend.DestBlend, hash);
			hash = hashValue(blend.BlendOp, hash);
			hash = hashValue(blend.SrcBlendAlpha, hash);
			hash = hashValue(blend.DestBlendAlpha, hash);
			hash = hashValue(blend.BlendOpAlpha, hash);
			hash = hashValue(blend.LogicOp, hash);
			hash = hashValue((uint32)blend.RenderTargetWriteMask, hash);   // UINT8 뒤 패딩 3바이트
		}

		hash = hashValue(inDesc.SampleMask, hash);

		// 래스터라이저는 필드가 모두 4바이트라 패딩이 없다.
		static_assert(sizeof(D3D12_RASTERIZER_DESC) == 11 * sizeof(uint32), "D3D12_RASTERIZER_DESC layout changed. hash it field by field");
		hash = HHash::HashState(&inDesc.RasterizerState, 1, hash);

		// 깊이 스텐실은 StencilReadMask/StencilWriteMask(UINT8) 뒤에 패딩 2바이트가 있다.
		const D3D12_DEPTH_STENCIL_DESC& depth = inDesc.DepthStencilState;
		hash = hashValue(depth.DepthEnable, hash);
		hash = hashValue(depth.DepthWriteMask, hash);
		hash = hashValue(depth.DepthFunc, hash);
		hash = hashValue(depth.StencilEnable, hash);
		hash = hashValue((uint32)depth.StencilReadMask, hash);
		hash = hashValue((uint32)depth.StencilWriteMask, hash);
		hash = hashStencilOp(depth.FrontFace, hash);
		hash = hashStencilOp(depth.BackFace, hash);

		// 입력 레이아웃은 포인터가 아니라 내용(시맨틱 이름 문자열 포함)으로.
		hash = hashValue((uint32)inElements.size(), hash);
		for (const D3D12_INPUT_ELEMENT_DESC& element : inElements)
		{
			if (element.SemanticName != nullptr)
			{
				hash = hashBytes(element.SemanticName, strlen(element.SemanticName), hash);
			}
			hash = hashValue(element.SemanticIndex, hash);
			hash = hashValue(element.Format, hash);
			hash = hashValue(element.InputSlot, hash);
			hash = hashValue(element.AlignedByteOffset, hash);
			hash = hashValue(element.InputSlotClass, hash);
			hash = hashValue(element.InstanceDataStepRate, hash);
		}

		hash = hashValue(inDesc.IBStripCutValue, hash);
		hash = hashValue(inDesc.PrimitiveTopologyType, hash);
		hash = hashValue(inDesc.NumRenderTargets, hash);
		for (uint32 i = 0; i < 8; ++i)
		{
			hash = hashValue(inDesc.RTVFormats[i], hash);
		}
		hash = hashValue(inDesc.DSVFormat, hash);
		hash = hashValue(inDesc.SampleDesc.Count, hash);
		hash = hashValue(inDesc.SampleDesc.Quality, hash);
		hash = hashValue(inDesc.NodeMask, hash);
		hash = hashValue(inDesc.Flags, hash);

		return hash;
	}
}

PGraphicsPipelineState::PGraphicsPipelineState()
{
	_desc.SampleMask			= UINT_MAX;
	_desc.RasterizerState		= CD3DX12_RASTERIZER_DESC(D3D12_DEFAULT);
	_desc.BlendState			= CD3DX12_BLEND_DESC(D3D12_DEFAULT);
	_desc.DepthStencilState     = CD3DX12_DEPTH_STENCIL_DESC(D3D12_DEFAULT);
	_desc.PrimitiveTopologyType = D3D12_PRIMITIVE_TOPOLOGY_TYPE_TRIANGLE;
	_desc.SampleDesc.Count      = 1;
	_desc.SampleDesc.Quality    = 0;
}

void PGraphicsPipelineState::BindRootSignature(const PRootSignature& rootSig)
{
	_bDirty = true;
	_desc.pRootSignature = rootSig.Get();
}

void PGraphicsPipelineState::BindRenderTarget(const HList<DXGI_FORMAT>& rtFormats, DXGI_FORMAT dvFormat)
{
	_bDirty = true;

	uint64 cnt = rtFormats.size();
	if (cnt > MAX_RENDERTARGET)
	{
		JG_LOG(Graphics, ELogLevel::Error, "RenderTarget Num can not exceed 8");
		cnt = MAX_RENDERTARGET;
	}

	_desc.NumRenderTargets = (uint32)cnt;
	for (uint64 i = 0; i < MAX_RENDERTARGET; ++i)
	{
		if (i < cnt)
		{
			_desc.RTVFormats[i] = rtFormats[i];
		}
		else
		{
			_desc.RTVFormats[i] = DXGI_FORMAT_UNKNOWN;
		}
	}
	_desc.DSVFormat = dvFormat;
}

void PGraphicsPipelineState::BindInputLayout(const HInputLayout& inputLayout)
{
	_bDirty = true;
	_inputLayoutDescs.clear();
	uint32 offset = 0;
	inputLayout.ForEach([&](const HInputElement& element)
	{
		D3D12_INPUT_ELEMENT_DESC Desc = {};
		Desc.SemanticIndex = element.SementicSlot;
		Desc.SemanticName  = element.SementicName;
		Desc.Format = HDirectX12Helper::ConvertDXGIFormat(element.Type);
		Desc.InputSlot = 0;
		Desc.AlignedByteOffset = offset;
		Desc.InstanceDataStepRate = 0;
		Desc.InputSlotClass = D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA;
		_inputLayoutDescs.push_back(Desc);
		offset += (uint32)(HJGGraphicsHelper::GetShaderDataTypeSize(element.Type));
	});

	_desc.InputLayout.NumElements = (uint32)_inputLayoutDescs.size();
	_desc.InputLayout.pInputElementDescs = _inputLayoutDescs.data();

}
void PGraphicsPipelineState::BindShader(const HHashMap<EShaderDomain, HList<uint8>>& inShaderBtDatas)
{
	_bDirty = true;

	for (const HPair<EShaderDomain, HList<uint8>>& dataPair : inShaderBtDatas)
	{
		switch (dataPair.first)
		{
		case EShaderDomain::Vertex:
			_desc.VS = {
				reinterpret_cast<const byte*>(dataPair.second.data()),
				dataPair.second.size()
			};
			break;
		case EShaderDomain::Domain:
			_desc.DS = {
				reinterpret_cast<const byte*>(dataPair.second.data()),
				dataPair.second.size()
			};
			break;
		case EShaderDomain::Hull:
			_desc.HS = {
				reinterpret_cast<const byte*>(dataPair.second.data()),
				dataPair.second.size()
			};
			break;
		case EShaderDomain::Geometry:
			_desc.GS = {
				reinterpret_cast<const byte*>(dataPair.second.data()),
				dataPair.second.size()
			};
			break;
		case EShaderDomain::Pixel:
			_desc.PS = {
				reinterpret_cast<const byte*>(dataPair.second.data()),
				dataPair.second.size()
			};
			break;
		}
	}
}

void PGraphicsPipelineState::SetPrimitiveTopologyType(D3D12_PRIMITIVE_TOPOLOGY_TYPE type)
{
	_bDirty = true;
	_desc.PrimitiveTopologyType = type;
}

void PGraphicsPipelineState::SetSampleMask(uint32_t sampleMask)
{
	_bDirty = true;
	_desc.SampleMask = sampleMask;
}

void PGraphicsPipelineState::SetRasterizerState(const D3D12_RASTERIZER_DESC& desc)
{
	_bDirty = true;
	_desc.RasterizerState = desc;
}

void PGraphicsPipelineState::SetBlendState(const D3D12_BLEND_DESC& desc)
{
	_bDirty = true;
	_desc.BlendState = desc;
}

void PGraphicsPipelineState::SetDepthStencilState(const D3D12_DEPTH_STENCIL_DESC& desc)
{
	_bDirty = true;
	_desc.DepthStencilState = desc;
}

bool PGraphicsPipelineState::Finalize()
{
	if (_bDirty == false && _dx12PSO != nullptr)
	{
		return true;
	}

	// 입력 레이아웃 포인터는 Finalize마다 현재 버퍼로 다시 잡는다. (BindInputLayout이 clear 후 다시 채운다)
	_desc.InputLayout.pInputElementDescs = _inputLayoutDescs.data();
	_desc.InputLayout.NumElements        = (uint32)_inputLayoutDescs.size();

	// 원본 바이트 해시가 아니라 내용 기반 키. 위 computeGraphicsPSOHash 주석 참고.
	const uint64 hash = computeGraphicsPSOHash(_desc, _inputLayoutDescs);

	HDX12Pipeline** PSORef = nullptr;
	bool bFirstCompile = false;

	{
		static HMutex s_HashMapMutex;
		HLockGuard<HMutex> lock(s_HashMapMutex);
		auto iter = HDirectXAPI::GetGraphicsPSOCacheRef().find(hash);

		if (iter == HDirectXAPI::GetGraphicsPSOCache().end())
		{
			bFirstCompile = true;
			PSORef = HDirectXAPI::GetGraphicsPSOCacheRef()[hash].GetAddressOf();
		}
		else
			PSORef = iter->second.GetAddressOf();
	}


	if (bFirstCompile)
	{
		HRESULT hr = HDirectXAPI::GetDevice()->CreateGraphicsPipelineState(&_desc, IID_PPV_ARGS(_dx12PSO.GetAddressOf()));
		if (FAILED(hr))
		{
			return false;
		}
		HDirectXAPI::GetGraphicsPSOCacheRef()[hash] = _dx12PSO;
		// 새 PSO는 드물어야 정상이다. 매 프레임 찍히면 캐시 키가 흔들리는 것이다.
		JG_LOG(Graphics, ELogLevel::Trace, "Graphics PSO created. cache size %d", (int32)HDirectXAPI::GetGraphicsPSOCache().size());
	}
	else
	{
		while (*PSORef == nullptr)
			std::this_thread::yield();
		_dx12PSO = *PSORef;
	}

	_bDirty = false;
	return true;
}

const D3D12_BLEND_DESC& PGraphicsPipelineState::GetBlendDesc() const
{
	return _desc.BlendState;
}



PComputePipelineState::PComputePipelineState()
{
}

void PComputePipelineState::BindRootSignature(const PRootSignature& rootSig)
{
	if (_desc.pRootSignature != rootSig.Get())
	{
		_bDirty = true;
	}

	_desc.pRootSignature = rootSig.Get();
}

void PComputePipelineState::BindShader(const PDX12ComputeShaderCompiler& shader)
{
	_bDirty = true;
	if (shader.GetCSData() != nullptr)
	{
		_desc.CS = {
			reinterpret_cast<byte*>(shader.GetCSData()->GetBufferPointer()),
			shader.GetCSData()->GetBufferSize()
		};
	}

}

bool PComputePipelineState::Finalize()
{
	if (_bDirty == false && _dx12PSO != nullptr)
	{
		return true;
	}

	uint64 hash = HHash::HashState(&_desc);

	ID3D12PipelineState** PSORef = nullptr;
	bool bFirstCompile = false;
	{
		static std::mutex s_HashMapMutex;
		std::lock_guard<std::mutex> CS(s_HashMapMutex);
		auto iter = HDirectXAPI::GetComputePSOCacheRef().find(hash);

		if (iter == HDirectXAPI::GetComputePSOCache().end())
		{
			bFirstCompile = true;
			PSORef = HDirectXAPI::GetComputePSOCacheRef()[hash].GetAddressOf();
		}
		else
			PSORef = iter->second.GetAddressOf();
	}

	if (bFirstCompile)
	{
		HRESULT hr = HDirectXAPI::GetDevice()->CreateComputePipelineState(&_desc, IID_PPV_ARGS(&_dx12PSO));
		if (FAILED(hr))
		{
			return false;
			assert("failed Create Compute PSO");
		}
		HDirectXAPI::GetComputePSOCacheRef()[hash] = _dx12PSO.Get();
	}
	else
	{
		while (*PSORef == nullptr)
			std::this_thread::yield();
		_dx12PSO = *PSORef;
	}

	_bDirty = false;
	return true;
}