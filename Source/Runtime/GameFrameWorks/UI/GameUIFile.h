#pragma once
#include "UI/GameUIDefines.h"

// 게임 UI 내부 전용. 글꼴 · 이미지 파일을 통째로 읽는다.
// HFileHelper::ReadAllBytes 를 거친다 — 패키지 빌드에서 Content · Saved · Bin 밖(예: OS 글꼴)을 읽으면 읽기 가드가 경고하고 스모크 보고서가 센다(ER-016).
namespace HGameUIFile
{
	inline bool ReadAllBytes(const PString& path, std::vector<uint8>& outData)
	{
		return HFileHelper::ReadAllBytes(path, outData);
	}
}
