#include "PCH/PCH.h"
#include "ConsoleCommandGlobalSystem.h"
#include "Misc/Log.h"
#include <algorithm>

namespace
{
	// Execute(토큰) 의 에코 줄. 공백이 든 토큰과 빈 토큰은 따옴표로 감싼다(표시용).
	PString joinTokensForEcho(const HList<PString>& InTokens)
	{
		PString line;
		for (uint64 i = 0; i < InTokens.size(); ++i)
		{
			if (i > 0)
			{
				line.Append(" ");
			}

			const PString& token = InTokens[i];
			if (token.Empty() || token.Contains(" "))
			{
				line.Append("\"").Append(token).Append("\"");
			}
			else
			{
				line.Append(token);
			}
		}

		return line;
	}

	bool isValidCommandName(const PString& InName)
	{
		if (InName.Empty())
		{
			return false;
		}

		for (const char c : InName.GetRawString())
		{
			if (c == ' ' || c == '\t' || c == '\r' || c == '\n' || c == '"')
			{
				return false;
			}
		}

		return true;
	}
}

// ----------------------------------------------------------------------------------------------------------------------------
// GConsoleCommandGlobalSystem
// ----------------------------------------------------------------------------------------------------------------------------

GConsoleCommandGlobalSystem::GConsoleCommandGlobalSystem()
{
	HConsoleCommandDesc helpDesc;
	helpDesc.Name        = "help";
	helpDesc.Usage       = "help [command]";
	helpDesc.Description = "List all commands, or show the usage of one command";
	helpDesc.Handler.BindRaw(this, &GConsoleCommandGlobalSystem::executeHelp);
	Register(helpDesc);

	// 레지스트리는 EXE 의 GCoreSystem::Create 가 만들므로, 이 생성자는 EXE 의 Core 사본이다 → EXE 에 선언된 명령을 등록한다.
	// (아직 GetInstance() 로 자기를 찾을 수 없는 시점이라 자기 참조를 넘긴다)
	HAutoConsoleCommand::RegisterAll(*this);
}

void GConsoleCommandGlobalSystem::Update()
{
	ExecutePending();
}

void GConsoleCommandGlobalSystem::Destroy()
{
	HAutoConsoleCommand::UnregisterAll(*this);
	Unregister("help");

	// 여기 남은 것은 Register 로 등록하고 해제하지 않은 명령이다. 핸들러가 해제된 모듈 객체나 PSharedPtr 를 잡고 있을 수 있다.
	for (const HPair<const PString, HConsoleCommandDesc>& pair : _commands)
	{
		JG_LOG(ConsoleCommand, ELogLevel::Warning, "Command '%s' was not unregistered by its owner", pair.first);
	}

	_commands.clear();
	_pendingLines.clear();
}

bool GConsoleCommandGlobalSystem::Register(const HConsoleCommandDesc& InDesc)
{
	const PString name = HConsoleCommandArgs::NormalizeName(InDesc.Name);
	if (isValidCommandName(name) == false)
	{
		JG_LOG(ConsoleCommand, ELogLevel::Error, "Register failed: invalid command name '%s'", name);
		return false;
	}

	if (InDesc.Handler.IsBound() == false)
	{
		JG_LOG(ConsoleCommand, ELogLevel::Error, "Register failed: '%s' has no handler", name);
		return false;
	}

	if (_commands.contains(name))
	{
		JG_LOG(ConsoleCommand, ELogLevel::Error, "Register failed: '%s' is already registered", name);
		return false;
	}

	HConsoleCommandDesc desc = InDesc;
	desc.Name = name;
	_commands.emplace(name, desc);
	return true;
}

bool GConsoleCommandGlobalSystem::Unregister(const PString& InName)
{
	return _commands.erase(HConsoleCommandArgs::NormalizeName(InName)) > 0;
}

bool GConsoleCommandGlobalSystem::IsRegistered(const PString& InName) const
{
	return _commands.contains(HConsoleCommandArgs::NormalizeName(InName));
}

bool GConsoleCommandGlobalSystem::Execute(const PString& InLine)
{
	HList<PString> tokens;
	PString error;
	if (HConsoleCommandArgs::Tokenize(InLine, tokens, error) == false)
	{
		JG_LOG(ConsoleCommand, ELogLevel::Info, "> %s", InLine);
		JG_LOG(ConsoleCommand, ELogLevel::Error, "%s", error);
		return false;
	}

	return executeTokens(tokens, InLine);
}

bool GConsoleCommandGlobalSystem::Execute(const HList<PString>& InTokens)
{
	return executeTokens(InTokens, joinTokensForEcho(InTokens));
}

void GConsoleCommandGlobalSystem::Submit(const PString& InLine)
{
	_pendingLines.push_back(InLine);
}

void GConsoleCommandGlobalSystem::ExecutePending()
{
	if (_pendingLines.empty())
	{
		return;
	}

	// 로컬로 옮긴 뒤 실행한다. 실행 중에 Submit 된 줄은 다음 번에 실행된다.
	HList<PString> lines;
	lines.swap(_pendingLines);

	for (const PString& line : lines)
	{
		Execute(line);
	}
}

bool GConsoleCommandGlobalSystem::executeTokens(const HList<PString>& InTokens, const PString& InEchoLine)
{
	if (InTokens.empty())
	{
		return true;
	}

	// 사용자가 친 문자열은 서식 자리에 넣지 않는다(AddLog 가 서식을 두 번 거친다).
	JG_LOG(ConsoleCommand, ELogLevel::Info, "> %s", InEchoLine);

	const HConsoleCommandArgs args(InTokens);
	HHashMap<PString, HConsoleCommandDesc>::const_iterator iter = _commands.find(args.GetName());
	if (iter == _commands.end())
	{
		JG_LOG(ConsoleCommand, ELogLevel::Error, "Unknown command '%s' (type help)", args.GetName());
		return false;
	}

	// 복사본으로 실행한다. 핸들러 안에서 Unregister 가 불려 _commands 의 원소가 지워져도 실행 중인 델리게이트가 살아 있다.
	const HConsoleCommandDelegate handler = iter->second.Handler;
	const PString usage = iter->second.Usage;

	if (handler.Execute(args) == false)
	{
		JG_LOG(ConsoleCommand, ELogLevel::Warning, "'%s' failed. Usage: %s", args.GetName(), usage);
		return false;
	}

	return true;
}

bool GConsoleCommandGlobalSystem::executeHelp(const HConsoleCommandArgs& InArgs)
{
	if (InArgs.GetPositionalCount() > 0)
	{
		const PString name = HConsoleCommandArgs::NormalizeName(InArgs.GetPositional(0));
		HHashMap<PString, HConsoleCommandDesc>::const_iterator iter = _commands.find(name);
		if (iter == _commands.end())
		{
			JG_LOG(ConsoleCommand, ELogLevel::Error, "Unknown command '%s'", name);
			return false;
		}

		JG_LOG(ConsoleCommand, ELogLevel::Info, "Usage: %s", iter->second.Usage);
		JG_LOG(ConsoleCommand, ELogLevel::Info, "  %s", iter->second.Description);
		return true;
	}

	HList<PString> names;
	names.reserve(_commands.size());
	for (const HPair<const PString, HConsoleCommandDesc>& pair : _commands)
	{
		names.push_back(pair.first);
	}

	std::sort(names.begin(), names.end(), [](const PString& InLeft, const PString& InRight)
		{
			return InLeft.GetRawString() < InRight.GetRawString();
		});

	JG_LOG(ConsoleCommand, ELogLevel::Info, "%llu commands (help <command> for usage):", static_cast<uint64>(names.size()));
	for (const PString& name : names)
	{
		JG_LOG(ConsoleCommand, ELogLevel::Info, "  %-28s %s", name, _commands.at(name).Description);
	}

	return true;
}

// ----------------------------------------------------------------------------------------------------------------------------
// HAutoConsoleCommand
// ----------------------------------------------------------------------------------------------------------------------------

HAutoConsoleCommand* HAutoConsoleCommand::Head = nullptr;

HAutoConsoleCommand::HAutoConsoleCommand(const char* InName, const char* InUsage, const char* InDescription, HandlerFunction InHandler)
	: _name(InName)
	, _usage(InUsage)
	, _description(InDescription)
	, _handler(InHandler)
	, _next(Head)
{
	Head = this;
}

void HAutoConsoleCommand::RegisterAll(GConsoleCommandGlobalSystem& InRegistry)
{
	for (HAutoConsoleCommand* command = Head; command != nullptr; command = command->_next)
	{
		if (command->_bRegistered)
		{
			continue;
		}

		if (command->_handler == nullptr)
		{
			JG_LOG(ConsoleCommand, ELogLevel::Error, "Register failed: '%s' has no handler", command->_name);
			continue;
		}

		HConsoleCommandDesc desc;
		desc.Name        = command->_name;
		desc.Usage       = command->_usage;
		desc.Description = command->_description;
		desc.Handler     = HConsoleCommandDelegate::CreateStatic(command->_handler);

		command->_bRegistered = InRegistry.Register(desc);
	}
}

void HAutoConsoleCommand::UnregisterAll(GConsoleCommandGlobalSystem& InRegistry)
{
	for (HAutoConsoleCommand* command = Head; command != nullptr; command = command->_next)
	{
		if (command->_bRegistered == false)
		{
			continue;
		}

		InRegistry.Unregister(command->_name);
		command->_bRegistered = false;
	}
}
