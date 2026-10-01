#include "PCH/PCH.h"
#include "UI/GameUIRenderer.h"
#include "UI/GameUIDrawList.h"
#include "UI/GameUIFont.h"
#include "JGGraphics.h"
#include "Classes/Texture.h"

bool PGameUIRenderer::Render(const HGameUIDrawList& drawList, const PSharedPtr<IRawTexture>& target)
{
	if (drawList.Commands.empty())
	{
		return true;
	}
	if (target.IsValid() == false || target->IsValid() == false)
	{
		return false;
	}

	const HTextureInfo& targetInfo = target->GetTextureInfo();
	if (EnumHasAnyFlags(targetInfo.Flags, ETextureFlags::Allow_RenderTarget) == false)
	{
		if (_bReportedTargetError == false)
		{
			_bReportedTargetError = true;
			JG_LOG(GameUI, ELogLevel::Error, "GameUI render target %s is not Allow_RenderTarget", targetInfo.Name);
		}
		return false;
	}

	HJGGraphicsModule* graphicsModule = GModuleGlobalSystem::GetInstance().FindModule<HJGGraphicsModule>();
	PSharedPtr<PJGGraphicsAPI> graphicsAPI = (graphicsModule != nullptr) ? graphicsModule->GetGraphicsAPI() : nullptr;
	if (graphicsAPI.IsValid() == false)
	{
		return false;
	}
	if (_graphicsCommand.IsValid() == false)
	{
		_graphicsCommand = graphicsAPI->GetGraphicsCommand();
	}

	const int32 width  = (int32)targetInfo.Width;
	const int32 height = (int32)targetInfo.Height;

	_drawCommands.clear();
	for (const HGameUIDrawCommand& command : drawList.Commands)
	{
		H2DDrawCommand drawCommand;
		drawCommand.Texture = command.Texture;
		if (drawCommand.Texture.IsValid() == false && command.Font != nullptr)
		{
			// 글꼴 아틀라스. 바뀌었으면 여기서 GPU 텍스처를 다시 만든다(업로드는 이번 프레임 드로우보다 먼저 실행된다).
			drawCommand.Texture = command.Font->GetAtlasTexture();
			if (drawCommand.Texture.IsValid() == false)
			{
				// 아틀라스가 없으면 흰 사각형이 되므로 그리지 않는다.
				continue;
			}
		}

		drawCommand.ClipLeft    = HMath::Clamp(HMath::FloorToInt(command.Clip.left), 0, width);
		drawCommand.ClipTop     = HMath::Clamp(HMath::FloorToInt(command.Clip.top), 0, height);
		drawCommand.ClipRight   = HMath::Clamp(HMath::CeilToInt(command.Clip.right), 0, width);
		drawCommand.ClipBottom  = HMath::Clamp(HMath::CeilToInt(command.Clip.bottom), 0, height);
		drawCommand.IndexOffset = command.IndexOffset;
		drawCommand.IndexCount  = command.IndexCount;
		_drawCommands.push_back(drawCommand);
	}
	if (_drawCommands.empty())
	{
		return true;
	}

	_graphicsCommand->BeginDraw();

	HRenderTarget renderTarget;
	renderTarget.RenderTextures[0] = target;
	renderTarget.Viewports.push_back(HViewport((float32)width, (float32)height));
	renderTarget.ScissorRects.push_back(HScissorRect(0, 0, width, height));
	_graphicsCommand->SetRenderTarget(renderTarget);

	H2DDrawArguments drawArgs;
	drawArgs.Vertices     = drawList.Vertices.data();
	drawArgs.VertexCount  = (uint64)drawList.Vertices.size();
	drawArgs.Indices      = drawList.Indices.data();
	drawArgs.IndexCount   = (uint64)drawList.Indices.size();
	drawArgs.Commands     = _drawCommands.data();
	drawArgs.CommandCount = (uint64)_drawCommands.size();
	drawArgs.TargetSize   = HVector2((float32)width, (float32)height);
	_graphicsCommand->Draw(drawArgs);

	_graphicsCommand->EndDraw();
	return true;
}
