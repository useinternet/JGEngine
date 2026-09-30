#pragma once
#include "{PROJECT_NAME}EditorDefines.h"
#include "Misc/Module.h"

// 에디터 환경(JGEditor)에서만 로드되는 모듈. 이 게임 전용 에디터 메뉴 · 위젯을 둔다.
class {PROJECT_NAME_UPPER}EDITOR_API H{PROJECT_NAME}EditorModule : public IModuleInterface
{
protected:
	virtual JGType GetModuleType() const override;
	virtual void StartupModule() override;
	virtual void ShutdownModule() override;
};
