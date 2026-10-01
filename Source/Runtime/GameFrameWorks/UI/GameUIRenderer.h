#pragma once
#include "UI/GameUIDefines.h"
#include "JGGraphicsDefine.h"

class IRawTexture;
class IJGGraphicsCommand;
class HGameUIDrawList;

// 그리기 목록을 렌더 타깃에 그린다(Graphics 2D 경로, 알파 블렌드, 타깃을 지우지 않는다). 메인 스레드 전용.
class GAMEFRAMEWORKS_API PGameUIRenderer : public IMemoryObject
{
	PSharedPtr<IJGGraphicsCommand> _graphicsCommand;   // 계속 쓴다(GetGraphicsCommand 는 호출마다 새 객체를 만든다)
	HList<H2DDrawCommand>          _drawCommands;
	bool                           _bReportedTargetError = false;

public:
	virtual ~PGameUIRenderer() = default;

	// target 은 Allow_RenderTarget 텍스처여야 한다. 그래픽 API 가 없거나 타깃이 맞지 않으면 false.
	bool Render(const HGameUIDrawList& drawList, const PSharedPtr<IRawTexture>& target);
};
