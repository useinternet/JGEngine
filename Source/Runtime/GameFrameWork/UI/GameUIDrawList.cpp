#include "PCH/PCH.h"
#include "UI/GameUIDrawList.h"
#include "Classes/Texture.h"

namespace
{
	bool isSameRect(const HRect& a, const HRect& b)
	{
		return a.left == b.left && a.top == b.top && a.right == b.right && a.bottom == b.bottom;
	}
}

void HGameUIDrawList::Clear()
{
	Vertices.clear();
	Indices.clear();
	Commands.clear();
	Transform = HGameUIAffine();
	Opacity   = 1.0f;
}

void HGameUIDrawList::AddQuad(const HRect& rect, const HRect& uv, const HLinearColor& color,
	const PSharedPtr<IRawTexture>& texture, PGameUIFont* font, const HRect& clip, int32 fontPage)
{
	if (rect.right <= rect.left || rect.bottom <= rect.top)
	{
		return;
	}

	const HVector2 corners[4] =
	{
		HVector2(rect.left, rect.top),
		HVector2(rect.right, rect.top),
		HVector2(rect.right, rect.bottom),
		HVector2(rect.left, rect.bottom),
	};
	AddQuad(corners, uv, color, texture, font, clip, fontPage);
}

void HGameUIDrawList::AddQuad(const HVector2 inputCorners[4], const HRect& uv, const HLinearColor& inputColor,
	const PSharedPtr<IRawTexture>& texture, PGameUIFont* font, const HRect& clip, int32 fontPage)
{
	// 지금 요소의 그리기 변환 · 불투명도(ER-014). 변환이 없으면 꼭짓점을 그대로 쓴다(지금까지와 같은 결과).
	const bool bTransformed = Transform.IsIdentity() == false;
	HVector2 corners[4];
	for (int32 i = 0; i < 4; ++i)
	{
		corners[i] = bTransformed ? Transform.Apply(inputCorners[i]) : inputCorners[i];
	}
	HLinearColor color = inputColor;
	if (Opacity < 1.0f)
	{
		color.A *= Opacity;
	}

	// 꼭짓점을 감싸는 상자가 클립 밖이면 넣지 않는다.
	float32 left   = corners[0].x;
	float32 right  = corners[0].x;
	float32 top    = corners[0].y;
	float32 bottom = corners[0].y;
	for (int32 i = 1; i < 4; ++i)
	{
		left   = HMath::Min(left, corners[i].x);
		right  = HMath::Max(right, corners[i].x);
		top    = HMath::Min(top, corners[i].y);
		bottom = HMath::Max(bottom, corners[i].y);
	}
	if (right <= clip.left || left >= clip.right || bottom <= clip.top || top >= clip.bottom)
	{
		return;
	}

	const bool bContinue = Commands.empty() == false &&
		Commands.back().Texture.GetRawPointer() == texture.GetRawPointer() &&
		Commands.back().Font == font &&
		Commands.back().FontPage == fontPage &&
		isSameRect(Commands.back().Clip, clip);
	if (bContinue == false)
	{
		HGameUIDrawCommand command;
		command.Texture     = texture;
		command.Font        = font;
		command.FontPage    = fontPage;
		command.Clip        = clip;
		command.IndexOffset = (uint32)Indices.size();
		Commands.push_back(command);
	}

	const uint32 baseVertex = (uint32)Vertices.size();
	const HVector2 texcoords[4] =
	{
		HVector2(uv.left, uv.top),
		HVector2(uv.right, uv.top),
		HVector2(uv.right, uv.bottom),
		HVector2(uv.left, uv.bottom),
	};

	H2DVertex vertex;
	vertex.Color = color;
	for (int32 i = 0; i < 4; ++i)
	{
		vertex.Position = corners[i];
		vertex.Texcoord = texcoords[i];
		Vertices.push_back(vertex);
	}

	const uint32 quadIndices[6] = { 0, 1, 2, 0, 2, 3 };
	for (uint32 index : quadIndices)
	{
		Indices.push_back(baseVertex + index);
	}
	Commands.back().IndexCount += 6;
}

uint64 HGameUIDrawList::GetQuadCount() const
{
	return (uint64)Vertices.size() / 4;
}
