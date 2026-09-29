#pragma once
#include "Core/GameFrameWorksDefines.h"
#include "Misc/Module.h"

class JGGameInstance;
class JGClass;

// 게임 프레임워크 모듈. 생명주기만 맡는다: 게임 인스턴스를 만들어 소유하고 프레임마다 틱하며 종료 시 정리한다.
// 월드 API 는 JGGameInstance 가 제공한다. Graphics · GUI 를 스스로 연결하지 않는다 (헤드리스 가능).
class GAMEFRAMEWORKS_API HGameFrameWorksModule : public IModuleInterface
{
	PSharedPtr<JGGameInstance> _gameInstance;
	std::chrono::steady_clock::time_point _lastTick;
	bool _bHasLastTick = false;
	bool _bShutdown    = false;

protected:
	virtual JGType GetModuleType() const override;
	virtual void StartupModule() override;
	virtual void ShutdownModule() override;

public:
	PSharedPtr<JGGameInstance> GetGameInstance() const;

	// 게임이 파생한 게임 인스턴스로 교체. 게임 모듈의 StartupModule 에서, 월드를 로드하기 전에 부른다.
	template<class T>
	PSharedPtr<T> ReplaceGameInstance()
	{
		PSharedPtr<T> instance = Allocate<T>();
		replaceGameInstance(instance);
		return instance;
	}

	PSharedPtr<JGGameInstance> ReplaceGameInstanceByClass(PSharedPtr<JGClass> instanceClass);

	// GameMaster 커널 자체 검증. 실패 수.
	int32 RunGameMasterSelfTest();

private:
	void tick();
	void replaceGameInstance(PSharedPtr<JGGameInstance> instance);
};
