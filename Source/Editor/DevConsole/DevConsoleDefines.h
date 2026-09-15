#pragma once
#include "Core.h"

#ifdef _DEVCONSOLE
#define DEVCONSOLE_API __declspec(dllexport)
#define DEVCONSOLE_C_API extern "C" __declspec(dllexport)
#else
#define DEVCONSOLE_API __declspec(dllimport)
#define DEVCONSOLE_C_API extern "C" __declspec(dllimport)
#endif

class DEVCONSOLE_API HDevConsoleArguments
{
private:
	PString Command;
	HHashMap<PString, PString> Params;

public:
	HDevConsoleArguments(const PString& InCommand)
	{
		HList<PString> Tokens = InCommand.Split(' ');
		uint32 NumTokens = (uint32)Tokens.size();

		if (Tokens.size() > 0)
		{
			Command = Tokens[0];
		}

		for (uint32 i = 1; i < NumTokens; ++i)
		{
			PString Token = Tokens[i].Trim();
			if (Token.Length() == 0 || Token[0] != '-')
			{
				continue;
			}

			Token.SubString(&Token, 1);

			HList<PString> ParamTokens = Token.Split('=');
			if (ParamTokens.size() == 1)
			{
				PString ParamName = ParamTokens[0];
				Params.emplace(ParamName.Trim(), "");
			}
			else if (ParamTokens.size() > 1)
			{
				PString ParamName = ParamTokens[0];
				PString ParamValue = ParamTokens[1];

				Params.emplace(ParamName.Trim(), ParamValue.Trim());
			}
		}
	}
public:
	const PString& GetCommand() const { return Command; }
	bool IsExist(const PString& InParamName) const { return Params.contains(InParamName); }
	bool TryGetParamValue(const PString& InParamName, PString& OutValue) const
	{
		OutValue.Reset();
		if (IsExist(InParamName) == false)
		{
			return false;
		}

		OutValue = Params.at(InParamName);
		return true;
	}
};

