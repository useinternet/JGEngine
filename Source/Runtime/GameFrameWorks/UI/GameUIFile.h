#pragma once
#include "UI/GameUIDefines.h"
#include <fstream>

// 게임 UI 내부 전용. 글꼴 · 이미지 파일을 통째로 읽는다.
// 한글 글꼴은 수 MB 라 엔진 풀 블록 한도(2MB)를 넘으므로 std 할당자를 쓴다. 경로는 wide 문자열로 연다(ASCII 가 아닌 경로도 열린다).
namespace HGameUIFile
{
	inline bool ReadAllBytes(const PString& path, std::vector<uint8>& outData)
	{
		outData.clear();

		std::ifstream file(path.GetRawWString(), std::ios::binary | std::ios::ate);
		if (file.is_open() == false)
		{
			return false;
		}

		const std::streamsize size = file.tellg();
		if (size <= 0)
		{
			return false;
		}

		outData.resize((size_t)size);
		file.seekg(0, std::ios::beg);
		file.read(reinterpret_cast<char*>(outData.data()), size);
		return file.good() || file.eof();
	}
}
