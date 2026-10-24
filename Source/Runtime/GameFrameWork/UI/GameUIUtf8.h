#pragma once
#include "UI/GameUIDefines.h"

// UTF-8 해석. 잘못된 바이트는 U+FFFD 로 바꾸고 한 바이트만 넘긴다(글자가 통째로 사라지지 않게).
namespace HGameUIUtf8
{
	constexpr uint32 ReplacementCharacter = 0xFFFD;

	// cursor 에서 코드포인트 하나를 읽고 커서를 옮긴다. 끝이면 false.
	inline bool Next(const char*& cursor, const char* end, uint32& outCodepoint)
	{
		if (cursor >= end)
		{
			return false;
		}

		const uint8 lead = (uint8)*cursor;
		int32  length    = 0;
		uint32 codepoint = 0;
		if (lead < 0x80)
		{
			outCodepoint = lead;
			cursor += 1;
			return true;
		}
		else if ((lead & 0xE0) == 0xC0)
		{
			length    = 2;
			codepoint = lead & 0x1F;
		}
		else if ((lead & 0xF0) == 0xE0)
		{
			length    = 3;
			codepoint = lead & 0x0F;
		}
		else if ((lead & 0xF8) == 0xF0)
		{
			length    = 4;
			codepoint = lead & 0x07;
		}
		else
		{
			outCodepoint = ReplacementCharacter;
			cursor += 1;
			return true;
		}

		if (end - cursor < length)
		{
			outCodepoint = ReplacementCharacter;
			cursor += 1;
			return true;
		}

		for (int32 i = 1; i < length; ++i)
		{
			const uint8 continuation = (uint8)cursor[i];
			if ((continuation & 0xC0) != 0x80)
			{
				outCodepoint = ReplacementCharacter;
				cursor += 1;
				return true;
			}
			codepoint = (codepoint << 6) | (continuation & 0x3F);
		}

		// 너무 긴 표현(overlong) · 서로게이트 · 범위 밖은 잘못된 값으로 본다.
		const uint32 minimumByLength[5] = { 0, 0, 0x80, 0x800, 0x10000 };
		if (codepoint < minimumByLength[length] || (codepoint >= 0xD800 && codepoint <= 0xDFFF) || codepoint > 0x10FFFF)
		{
			outCodepoint = ReplacementCharacter;
			cursor += 1;
			return true;
		}

		outCodepoint = codepoint;
		cursor += length;
		return true;
	}

	// 코드포인트 하나를 UTF-8 로 out 에 쓰고 바이트 수를 돌려준다(0 으로 끝맺는다). 서로게이트 · 범위 밖은 U+FFFD.
	inline int32 Encode(uint32 codepoint, char out[5])
	{
		if ((codepoint >= 0xD800 && codepoint <= 0xDFFF) || codepoint > 0x10FFFF)
		{
			codepoint = ReplacementCharacter;
		}

		int32 length = 0;
		if (codepoint < 0x80)
		{
			out[0] = (char)codepoint;
			length = 1;
		}
		else if (codepoint < 0x800)
		{
			out[0] = (char)(0xC0 | (codepoint >> 6));
			out[1] = (char)(0x80 | (codepoint & 0x3F));
			length = 2;
		}
		else if (codepoint < 0x10000)
		{
			out[0] = (char)(0xE0 | (codepoint >> 12));
			out[1] = (char)(0x80 | ((codepoint >> 6) & 0x3F));
			out[2] = (char)(0x80 | (codepoint & 0x3F));
			length = 3;
		}
		else
		{
			out[0] = (char)(0xF0 | (codepoint >> 18));
			out[1] = (char)(0x80 | ((codepoint >> 12) & 0x3F));
			out[2] = (char)(0x80 | ((codepoint >> 6) & 0x3F));
			out[3] = (char)(0x80 | (codepoint & 0x3F));
			length = 4;
		}
		out[length] = 0;
		return length;
	}

	// 코드포인트 목록 → UTF-8 글. 0 은 넣지 않는다(글이 거기서 끝난다).
	inline PString Encode(const HList<uint32>& codepoints)
	{
		std::string bytes;
		bytes.reserve(codepoints.size() * 3);
		char buffer[5];
		for (uint32 codepoint : codepoints)
		{
			const int32 length = Encode(codepoint, buffer);
			bytes.append(buffer, (size_t)length);
		}
		return PString(bytes.c_str());
	}
}
