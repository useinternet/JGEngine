#include "PCH/PCH.h"
#include "{PROJECT_NAME}EntryActor.h"

void JG{PROJECT_NAME}EntryActor::OnEnterWorld()
{
	// 멀티플레이 규약 (싱글도 같은 코드가 돈다. 실행 인자 -host · -join 이면 리슨 서버 · 클라):
	//   규칙 · 컴포넌트 등록은 모든 기계에서, 초기 상태 구성과 시작은 권한 쪽(IsAuthority)에서만. 클라는 호스트가 보낸 초기 상태로 시작한다.
	//
	//   PSharedPtr<JGGameMasterActor> gameMasterActor = GetWorld()->SpawnActor<JGGameMasterActor>(PName("GameMaster"));
	//   PSharedPtr<PGameMaster>       gameMaster      = Allocate<PGameMaster>();
	//   gameMaster->RegisterHandler(...); gameMaster->RegisterComponent<...>(...); gameMaster->SetTeamOfActorFunction(...);
	//   gameMasterActor->SetGameMaster(gameMaster);
	//   if (GetGameInstance().IsAuthority() == true)
	//   {
	//       HGameplayState& initial = gameMaster->EditInitialState();   // 초기 상태 구성
	//       gameMasterActor->StartGame(seed);
	//   }
	JG_LOG({PROJECT_NAME}, ELogLevel::Info, "{PROJECT_NAME} entry actor entered world");
}
