#pragma once
#include "Core.h"
#include "Misc/Module.h"
#include "DevConsoleDefines.h"
#include "DevConsole.h"

// DevConsole 모듈: 메뉴 Windows/DevConsole 로 콘솔 창을 연다.
// 명령 등록·실행은 Core 의 GConsoleCommandGlobalSystem 이 맡는다(이 모듈에는 명령 API 가 없다).
class DEVCONSOLE_API HDevConsoleModule : public IModuleInterface
{
public:
	virtual ~HDevConsoleModule();

protected:
	virtual JGType GetModuleType() const override;

	virtual void StartupModule() override;
	virtual void ShutdownModule() override;
};
