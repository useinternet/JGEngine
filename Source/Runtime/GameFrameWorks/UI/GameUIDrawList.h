#pragma once
#include "UI/GameUIDefines.h"
#include "JGGraphicsDefine.h"

class IRawTexture;
class PGameUIFont;

// 한 배치. 같은 텍스처(또는 같은 글꼴 아틀라스 페이지)와 같은 클립으로 이어진 사각형 묶음. 둘 다 없으면 단색(기본 흰 텍스처).
struct HGameUIDrawCommand
{
	PSharedPtr<IRawTexture> Texture;
	// 글꼴 아틀라스. 그리기 직전에 GPU 텍스처로 바뀐다. 목록을 만든 그 프레임 동안만 쓴다(글꼴은 관리자 · 요소가 들고 있다).
	PGameUIFont*            Font     = nullptr;
	int32                   FontPage = 0;   // 글꼴 아틀라스 페이지
	HRect                   Clip;           // 렌더 타깃 픽셀
	uint32                  IndexOffset = 0;
	uint32                  IndexCount  = 0;
};

// 화면 한 장의 그리기 목록. 좌표는 렌더 타깃 픽셀. 사각형 하나 = 정점 4 · 인덱스 6.
class GAMEFRAMEWORKS_API HGameUIDrawList
{
public:
	// 큰 목록도 받도록 std 할당자(엔진 풀 블록 한도 2MB).
	std::vector<H2DVertex>    Vertices;
	std::vector<uint32>       Indices;
	HList<HGameUIDrawCommand> Commands;

	// 지금 그리는 요소의 그리기 변환(렌더 타깃 픽셀) · 불투명도. AddQuad 가 모든 꼭짓점 · 정점 알파에 적용한다.
	// 요소 그리기(PGameUIElement::buildDraw)가 요소마다 정한다. Clear 가 단위 변환 · 1 로 되돌린다.
	HGameUIAffine Transform;
	float32       Opacity = 1.0f;

public:
	void Clear();
	// rect(픽셀)에 uv 영역을 그린다. clip 밖은 시저로 잘리고, 완전히 밖이면 넣지 않는다.
	// 바로 앞 배치와 텍스처 · 글꼴(페이지) · 클립이 같으면 그 배치에 이어 붙인다.
	void AddQuad(const HRect& rect, const HRect& uv, const HLinearColor& color,
		const PSharedPtr<IRawTexture>& texture, PGameUIFont* font, const HRect& clip, int32 fontPage = 0);
	// 네 꼭짓점(왼쪽 위 · 오른쪽 위 · 오른쪽 아래 · 왼쪽 아래)에 uv 사각형을 그린다(비튼 사각형 — 합성 기울임 글자).
	void AddQuad(const HVector2 corners[4], const HRect& uv, const HLinearColor& color,
		const PSharedPtr<IRawTexture>& texture, PGameUIFont* font, const HRect& clip, int32 fontPage = 0);
	uint64 GetQuadCount() const;
};
