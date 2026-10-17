#include "PCH/PCH.h"
#include "DX12FrameBuffer.h"
#include "DX12Texture.h"
#include "DirectX12API.h"
#include "JGGraphicsHelper.h"
#include "Classes/CommandQueue.h"
#include "Classes/CommandList.h"

PDX12FrameBuffer::~PDX12FrameBuffer()
{
}
const PName& PDX12FrameBuffer::GetName() const
{
	return _name;
}

void PDX12FrameBuffer::SetName(const PName& inName)
{
	_name = inName;
}


bool PDX12FrameBuffer::Initialize(const HFrameBufferInfo& info)
{
	_name = PName("DX12FrameBuffer");
	_frameBufferInfo = info;

	_dx12SwapChain = HDirectX12Helper::CreateDXGISwapChain(
		(HWND)_frameBufferInfo.Handle,
		HDirectXAPI::GetFactory(),
		HDirectXAPI::GetCommandQueue()->Get(),
		HDirectX12Helper::ConvertDXGIFormat(_frameBufferInfo.Format),
		_frameBufferInfo.Width, _frameBufferInfo.Height,
		_frameBufferInfo.FrameBufferCount);

	if (_dx12SwapChain == nullptr)
	{
		JG_LOG(Graphics, ELogLevel::Error, "Failed Create SwapChain in DirectX12RenderContext");
		return false;
	}

	_dx12BackBuffers.resize(_frameBufferInfo.FrameBufferCount);
	_rtvs.resize(_frameBufferInfo.FrameBufferCount);

	for (uint64 i = 0; i < _frameBufferInfo.FrameBufferCount; ++i)
	{
		_rtvs[i] = HDirectXAPI::RTVAllocate();

		_dx12SwapChain->GetBuffer((uint32)i, IID_PPV_ARGS(_dx12BackBuffers[i].GetAddressOf()));

		HDirectXAPI::GetDevice()->CreateRenderTargetView(_dx12BackBuffers[i].Get(), nullptr, _rtvs[i].CPU());

		_dx12BackBuffers[i]->SetName((TEXT("FrameBufferResource_") + std::to_wstring(i)).c_str());
	}

	return true;
}

void PDX12FrameBuffer::Update()
{
	uint32 index = _dx12SwapChain->GetCurrentBackBufferIndex();

	HDX12ComPtr<HDX12Resource>& backBuffer = _dx12BackBuffers[index];

	PSharedPtr<PCommandQueue> commandQueue = HDirectXAPI::GetCommandQueue();
	PSharedPtr<PCommandList>  commandList  = commandQueue->RequestCommandList(ECommandListType::Base, ECommandListPriority::Present);

	CD3DX12_RESOURCE_BARRIER resourceBarrier = CD3DX12_RESOURCE_BARRIER::Transition(backBuffer.Get(), D3D12_RESOURCE_STATE_PRESENT, D3D12_RESOURCE_STATE_RENDER_TARGET);
	commandList->Get()->ResourceBarrier(1, &resourceBarrier);

	float32 clearColor[4] = { _frameBufferInfo.ClearColor.R,_frameBufferInfo.ClearColor.G, _frameBufferInfo.ClearColor.B, _frameBufferInfo.ClearColor.A};
	commandList->Get()->ClearRenderTargetView(_rtvs[index].CPU(), clearColor, 0, nullptr);

	// 제출 텍스처의 크기 · 포맷이 백 버퍼와 다르면 CopyResource 가 D3D 오류다. 그 프레임은 복사하지 않는다. (GP-G3, 배율 블릿은 GP-H9)
	// 창 크기를 바꾼 직후 게임 화면 텍스처가 아직 옛 크기인 동안 생기는 일이라, 맞지 않는 구간마다 한 번만 남긴다.
	bool bCopySubmittedTexture = false;
	if (_submittedTexture.IsValid() == true)
	{
		const HTextureInfo& texInfo = _submittedTexture->GetTextureInfo();
		bCopySubmittedTexture = texInfo.Width == _frameBufferInfo.Width && texInfo.Height == _frameBufferInfo.Height && texInfo.Format == _frameBufferInfo.Format;
		if (bCopySubmittedTexture)
		{
			_bSubmitMismatchLogged = false;
		}
		else if (_bSubmitMismatchLogged == false)
		{
			_bSubmitMismatchLogged = true;
			JG_LOG(Graphics, ELogLevel::Info, "FrameBuffer : submitted texture %s (%dx%d, %s) does not match the back buffer (%dx%d, %s). Copy skipped until they match",
				texInfo.Name, (int32)texInfo.Width, (int32)texInfo.Height, StaticEnum<ETextureFormat>()->GetEnumNameByValue((int32)texInfo.Format),
				(int32)_frameBufferInfo.Width, (int32)_frameBufferInfo.Height, StaticEnum<ETextureFormat>()->GetEnumNameByValue((int32)_frameBufferInfo.Format));
		}
	}

	D3D12_RESOURCE_STATES state = D3D12_RESOURCE_STATE_RENDER_TARGET;
	if (bCopySubmittedTexture)
	{
		CD3DX12_RESOURCE_BARRIER resourceBarrier = CD3DX12_RESOURCE_BARRIER::Transition(
			backBuffer.Get(), D3D12_RESOURCE_STATE_RENDER_TARGET, D3D12_RESOURCE_STATE_COPY_DEST);
		commandList->Get()->ResourceBarrier(1, &resourceBarrier);

		commandList->TransitionBarrier(_submittedTexture->Get(), D3D12_RESOURCE_STATE_COPY_SOURCE);
		commandList->FlushResourceBarrier();

		commandList->Get()->CopyResource(backBuffer.Get(), _submittedTexture->Get());
		state = D3D12_RESOURCE_STATE_COPY_DEST;
	}

	OnUpdate.BroadCast(commandList->Get(), backBuffer.Get(), _rtvs[index].CPU(), state);

	resourceBarrier = CD3DX12_RESOURCE_BARRIER::Transition(backBuffer.Get(), state, D3D12_RESOURCE_STATE_PRESENT);
	commandList->Get()->ResourceBarrier(1, &resourceBarrier);
}

bool PDX12FrameBuffer::Present()
{
	OnPresent.BroadCast();

	// 최소화(0 x 0) 중에는 내보내지 않는다. 프레임 기록 · 제출 · 펜스는 그대로 돈다. (GP-G1)
	if (_bMinimized)
	{
		return true;
	}

	HRESULT hResult = _dx12SwapChain->Present(_bVSyncEnabled ? 1 : 0, 0);
	return SUCCEEDED(hResult);
}

void PDX12FrameBuffer::SubmitTexture(PSharedPtr<IRawTexture> inTexture)
{
	_submittedTexture = Cast<PDX12Texture>(inTexture);
}

void PDX12FrameBuffer::Resize(uint32 width, uint32 height)
{
	// 최소화되면 창이 0 x 0 을 보낸다. 버퍼를 1 x 1 로 줄이지 않고 그대로 둔 채 Present 만 멈춘다. (GP-G1)
	if (width == 0 || height == 0)
	{
		if (_bMinimized == false)
		{
			_bMinimized = true;
			JG_LOG(Graphics, ELogLevel::Trace, "FrameBuffer : window minimized. Present paused");
		}
		return;
	}

	if (_bMinimized)
	{
		_bMinimized = false;
		JG_LOG(Graphics, ELogLevel::Trace, "FrameBuffer : window restored (%dx%d)", (int32)width, (int32)height);
	}

	if (_frameBufferInfo.Width == width && _frameBufferInfo.Height == height)
	{
		return;
	}

	// 백 버퍼를 GPU 가 다 쓴 뒤에 놓는다. 크기 변경은 창 메시지(프레임 밖)에서 오므로 기록 중인 프레임은 없다.
	HDirectXAPI::GetCommandQueue()->Flush();

	for (uint64 i = 0; i < _frameBufferInfo.FrameBufferCount; ++i)
	{
		_dx12BackBuffers[i].Reset();
	}

	DXGI_SWAP_CHAIN_DESC1 desc;
	_dx12SwapChain->GetDesc1(&desc);
	desc.Width  = width;
	desc.Height = height;

	const HRESULT hResult = _dx12SwapChain->ResizeBuffers((uint32)_frameBufferInfo.FrameBufferCount, desc.Width, desc.Height, desc.Format, desc.Flags);
	if (FAILED(hResult))
	{
		JG_LOG(Graphics, ELogLevel::Error, "FrameBuffer : ResizeBuffers(%dx%d) failed (0x%08X). Keeping %dx%d",
			(int32)width, (int32)height, (uint32)hResult, (int32)_frameBufferInfo.Width, (int32)_frameBufferInfo.Height);
	}
	else
	{
		_frameBufferInfo.Width  = desc.Width;
		_frameBufferInfo.Height = desc.Height;
	}

	for (uint64 i = 0; i < _frameBufferInfo.FrameBufferCount; ++i)
	{
		_dx12SwapChain->GetBuffer((uint32)i, IID_PPV_ARGS(_dx12BackBuffers[i].GetAddressOf()));
		HDirectXAPI::GetDevice()->CreateRenderTargetView(_dx12BackBuffers[i].Get(), nullptr, _rtvs[i].CPU());
		_dx12BackBuffers[i]->SetName((TEXT("FrameBufferResource_") + std::to_wstring(i)).c_str());
	}

	JG_LOG(Graphics, ELogLevel::Trace, "FrameBuffer : resized to %dx%d", (int32)_frameBufferInfo.Width, (int32)_frameBufferInfo.Height);
}

const HFrameBufferInfo& PDX12FrameBuffer::GetInfo() const
{
	return _frameBufferInfo;
}

void PDX12FrameBuffer::SetVSyncEnabled(bool inEnabled)
{
	if (_bVSyncEnabled != inEnabled)
	{
		_bVSyncEnabled = inEnabled;
		JG_LOG(Graphics, ELogLevel::Info, "FrameBuffer : VSync %s", PString(inEnabled ? "on" : "off"));
	}
}

bool PDX12FrameBuffer::IsVSyncEnabled() const
{
	return _bVSyncEnabled;
}

bool PDX12FrameBuffer::IsMinimized() const
{
	return _bMinimized;
}
