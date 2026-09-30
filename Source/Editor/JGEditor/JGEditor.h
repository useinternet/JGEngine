#pragma once
#include "Core.h"
#include "Misc/Module.h"
#include "JGEditorDefine.h"

class PJWindow;

// 에디터 호스트(런처의 LaunchModule). 창을 열고 엔진 에디터 모듈을 연결한 뒤,
// 게임 프로젝트로 실행됐으면 .jgproject 의 게임 모듈 → 에디터 모듈 순으로 올린다.
class JGEDITOR_API HJGEditorModule : public IModuleInterface
{
	PSharedPtr<PJWindow> _window;
	HList<PString>       _connectedProjectModules;   // 연결한 순서. 종료는 역순

public:
	virtual ~HJGEditorModule() = default;

protected:
	virtual JGType GetModuleType() const override;

	virtual void StartupModule() override;
	virtual void ShutdownModule() override;

private:
	void openDefaultWidgets();
	void connectProjectModules();
	void disconnectProjectModules();
};
