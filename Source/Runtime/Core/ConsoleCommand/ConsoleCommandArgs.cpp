#include "PCH/PCH.h"
#include "ConsoleCommandArgs.h"
#include <charconv>

namespace
{
	bool isWhiteSpace(char InChar)
	{
		return InChar == ' ' || InChar == '\t' || InChar == '\r' || InChar == '\n';
	}
}

HConsoleCommandArgs::HConsoleCommandArgs(const HList<PString>& InTokens)
{
	const uint64 numTokens = InTokens.size();
	if (numTokens == 0)
	{
		return;
	}

	_name = NormalizeName(InTokens[0]);

	for (uint64 i = 1; i < numTokens; ++i)
	{
		const PString& token = InTokens[i];

		// "-" 한 글자는 위치 인자로 둔다.
		if (token.Length() < 2 || token[0] != '-')
		{
			_positionals.push_back(token);
			continue;
		}

		PString key;
		PString value;
		const uint64 equalPos = token.Find("=");
		if (equalPos == PString::NPOS)
		{
			token.SubString(&key, 1);
		}
		else
		{
			token.SubString(&key, 1, equalPos - 1);
			token.SubString(&value, equalPos + 1);
		}

		// "-=value" 처럼 이름이 없으면 위치 인자로 둔다.
		if (key.Empty())
		{
			_positionals.push_back(token);
			continue;
		}

		_options[NormalizeName(key)] = value;
	}
}

bool HConsoleCommandArgs::Tokenize(const PString& InLine, HList<PString>& OutTokens, PString& OutError)
{
	OutTokens.clear();
	OutError.Reset();

	const HRawString& line = InLine.GetRawString();

	HRawString current;
	bool bInQuote  = false;
	bool bHasToken = false; // "" 처럼 비어 있는 따옴표도 토큰 하나로 센다.

	for (const char c : line)
	{
		if (c == '"')
		{
			bInQuote  = !bInQuote;
			bHasToken = true;
			continue;
		}

		if (bInQuote == false && isWhiteSpace(c))
		{
			if (bHasToken)
			{
				OutTokens.push_back(PString(current.c_str()));
				current.clear();
				bHasToken = false;
			}
			continue;
		}

		current.push_back(c);
		bHasToken = true;
	}

	if (bInQuote)
	{
		OutTokens.clear();
		OutError = "Unterminated quote";
		return false;
	}

	if (bHasToken)
	{
		OutTokens.push_back(PString(current.c_str()));
	}

	return true;
}

PString HConsoleCommandArgs::NormalizeName(const PString& InName)
{
	HRawString result = InName.GetRawString();
	for (char& c : result)
	{
		if (c >= 'A' && c <= 'Z')
		{
			c = static_cast<char>(c - 'A' + 'a');
		}
	}

	return PString(result.c_str());
}

const PString& HConsoleCommandArgs::GetName() const
{
	return _name;
}

uint32 HConsoleCommandArgs::GetPositionalCount() const
{
	return static_cast<uint32>(_positionals.size());
}

PString HConsoleCommandArgs::GetPositional(uint32 InIndex) const
{
	if (InIndex >= _positionals.size())
	{
		return PString();
	}

	return _positionals[InIndex];
}

bool HConsoleCommandArgs::Has(const PString& InName) const
{
	return _options.contains(NormalizeName(InName));
}

bool HConsoleCommandArgs::TryGetString(const PString& InName, PString& OutValue) const
{
	HHashMap<PString, PString>::const_iterator iter = _options.find(NormalizeName(InName));
	if (iter == _options.end())
	{
		return false;
	}

	OutValue = iter->second;
	return true;
}

bool HConsoleCommandArgs::TryGetInt(const PString& InName, int32& OutValue) const
{
	HHashMap<PString, PString>::const_iterator iter = _options.find(NormalizeName(InName));
	if (iter == _options.end())
	{
		return false;
	}

	const HRawString& text = iter->second.GetRawString();
	if (text.empty())
	{
		return false;
	}

	// std::from_chars 는 예외를 던지지 않는다. 문자열 전체를 소비해야 성공으로 본다("12x" 는 실패).
	const char* first = text.data();
	const char* last  = text.data() + text.size();

	int32 value = 0;
	const std::from_chars_result result = std::from_chars(first, last, value);
	if (result.ec != std::errc() || result.ptr != last)
	{
		return false;
	}

	OutValue = value;
	return true;
}
