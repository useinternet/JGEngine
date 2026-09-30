#pragma once
#include "MyGameEditorDefines.h"
#include "Misc/Module.h"

// 에디터 환경에서만 로드되는 모듈. 이 게임 전용 에디터 메뉴 · 위젯을 둔다.
class MYGAMEEDITOR_API HMyGameEditorModule : public IModuleInterface
{
protected:
	virtual JGType GetModuleType() const override;
	virtual void StartupModule() override;
	virtual void ShutdownModule() override;
};
