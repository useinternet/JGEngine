#pragma once
#include "GameMaster/Messages/GameplayCommand.h"

// 실행된 명령의 기록. 초기 상태 + 이 로그 = 리플레이.
class GAMEFRAMEWORKS_API PGameplayCommandLog : public IJsonable
{
public:
	HList<HGameplayCommand> Commands;

	void  Append(const HGameplayCommand& command);
	bool  PopLast();
	int32 Count() const;
	void  Clear();

protected:
	virtual void WriteJson(PJsonData& json) const override;
	virtual void ReadJson(const PJsonData& json) override;
};
