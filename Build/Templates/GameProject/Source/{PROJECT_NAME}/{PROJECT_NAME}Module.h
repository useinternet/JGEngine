#pragma once
#include "{PROJECT_NAME}Defines.h"
#include "Misc/Module.h"

// 게임 코드 모듈. 게임 실행과 에디터 환경 모두에서 로드된다.
class {PROJECT_NAME_UPPER}_API H{PROJECT_NAME}Module : public IModuleInterface
{
protected:
	virtual JGType GetModuleType() const override;
	virtual void StartupModule() override;
	virtual void ShutdownModule() override;
};
