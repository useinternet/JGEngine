#pragma once
#include "Core/GameFrameWorksDefines.h"
#include "GameMaster/GameMasterDefines.h"
#include "GameMaster/State/GameplayEntityId.h"

// GameMaster 보드(정수 칸 좌표)를 월드에 놓는 방법. 규칙은 칸 좌표만 알고, 칸이 월드 어디에 있는지는 이 값이 정한다.
// 보드 평면은 월드 수평면(+Y 가 위)이고 Origin 이 칸 (0, 0) 의 중심이다. 칸 X 는 월드 +X, 칸 Y 는 월드 +Z 쪽으로 늘어난다.
//   Square · Free : 칸 중심 = Origin + (X, 0, Y) · CellSize
//   Hex           : 축 좌표 (q = X, r = Y), 꼭짓점이 ±Z 를 향하는 육각(pointy-top).
//                   칸 중심 = Origin + (CellSize · (q + r / 2), 0, CellSize · r · √3 / 2)
// CellSize 는 이웃한 두 칸 중심 사이 거리다. 보드 범위(Width · Height)는 보지 않는다 — HGameplayBoardState::InBounds 로 거른다.
struct GAMEFRAMEWORKS_API HGameplayBoardLayout
{
	EGameplayBoardKind Kind     = EGameplayBoardKind::None;
	HVector3           Origin;
	float32            CellSize = 1.0f;

	HGameplayBoardLayout() = default;
	HGameplayBoardLayout(EGameplayBoardKind inKind, const HVector3& inOrigin, float32 inCellSize);

	// 보드가 있고 CellSize > 0.
	bool IsValid() const;

	HVector3 CoordToWorld(const HGameplayCoord& coord) const;
	// 월드 점(높이는 무시)을 담는 칸.
	bool WorldToCoord(const HVector3& position, HGameplayCoord* outCoord) const;
	// 광선이 보드 평면과 만나는 점과 그 칸. 평면과 평행하거나 평면이 광선 뒤쪽이면 false.
	bool PickCoord(const HRay& ray, HGameplayCoord* outCoord, HVector3* outPosition) const;
};
