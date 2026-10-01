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
}

void HGameUIDrawList::AddQuad(const HRect& rect, const HRect& uv, const HLinearColor& color,
	const PSharedPtr<IRawTexture>& texture, PGameUIFont* font, const HRect& clip)
{
	if (rect.right <= rect.left || rect.bottom <= rect.top)
	{
		return;
	}
	if (rect.right <= clip.left || rect.left >= clip.right || rect.bottom <= clip.top || rect.top >= clip.bottom)
	{
		return;
	}

	const bool bContinue = Commands.empty() == false &&
		Commands.back().Texture.GetRawPointer() == texture.GetRawPointer() &&
		Commands.back().Font == font &&
		isSameRect(Commands.back().Clip, clip);
	if (bContinue == false)
	{
		HGameUIDrawCommand command;
		command.Texture     = texture;
		command.Font        = font;
		command.Clip        = clip;
		command.IndexOffset = (uint32)Indices.size();
		Commands.push_back(command);
	}

	const uint32 baseVertex = (uint32)Vertices.size();

	H2DVertex vertex;
	vertex.Color = color;

	vertex.Position = HVector2(rect.left, rect.top);
	vertex.Texcoord = HVector2(uv.left, uv.top);
	Vertices.push_back(vertex);

	vertex.Position = HVector2(rect.right, rect.top);
	vertex.Texcoord = HVector2(uv.right, uv.top);
	Vertices.push_back(vertex);

	vertex.Position = HVector2(rect.right, rect.bottom);
	vertex.Texcoord = HVector2(uv.right, uv.bottom);
	Vertices.push_back(vertex);

	vertex.Position = HVector2(rect.left, rect.bottom);
	vertex.Texcoord = HVector2(uv.left, uv.bottom);
	Vertices.push_back(vertex);

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
