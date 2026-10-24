#pragma once
#include "Core.h"
#include "JGGraphicsDefine.h"
#include "Classes/Scene.h"

class PSceneRenderer;
class IMesh;
class IRawTexture;
class IRawMaterial;

// 렌더 결과 자체 시험 (graphics.selftest, 사용자 결정 H-071 · H-083).
// GameFrameWork · 에셋 없이 Graphics 공개 API 만으로 장면을 만들어 그리고, 리드백한 픽셀을 미리 계산한 값과 비교한다.
//  장면: 카메라 (0, 0, -5) → +Z. 한 변 1 상자 메시 하나를 두 번 놓는다.
//        앞 상자 = 원점, 1x1 텍스처 머터리얼로 덮어쓰기. 뒤 상자 = 2배 · (1, 0, 3), 덮어쓰기 없음(기본 머터리얼 = 흰색).
//  흐름: Start(명령, 프레임 밖) → Update 단계 tick 마다 Render → RenderFrameCount 프레임 뒤 리드백 → 콜백 → 판정 → 정리.
// HJGGraphicsModule 이 하나를 들고 있다. 메인 스레드 전용.
class PRenderSelfTest : public IMemoryObject
{
	enum class EState
	{
		Idle,
		Rendering,
		WaitReadback,
		Done,
	};

	EState _state = EState::Idle;
	bool   _bExitWhenDone = false;
	int32  _failures = 0;
	int32  _passed   = 0;
	int32  _renderedFrames = 0;
	int32  _framesSinceReadback = 0;
	uint64 _logSerialAtStart = 0;

	PSharedPtr<PScene>         _scene;
	PSharedPtr<PSceneRenderer> _renderer;
	PSharedPtr<IMesh>          _cubeMesh;
	PSharedPtr<IRawTexture>    _texture;
	PSharedPtr<IRawMaterial>   _texturedMaterial;
	HSceneCameraID             _camera;

	// 리드백 결과. 알베도는 비동기(콜백), 출력 · 디스플레이는 즉시.
	HTexturePixels _albedoPixels;
	HTexturePixels _outputPixels;
	HTexturePixels _displayPixels;
	bool _bAlbedoArrived = false;
	bool _bImmediateRead = false;

public:
	virtual ~PRenderSelfTest() = default;

	// 자원 · 장면을 만들고 Update 단계 tick 을 예약한다. 만들기에 실패하면 결과를 남기고 바로 끝낸다(그래도 true — 결과는 로그 · 종료 코드로).
	// bExitWhenDone: 끝나면 GCoreSystem::RequestExit(실패 0 이면 0, 아니면 1). JGConsole -graphics 용.
	bool Start(bool bExitWhenDone);
	bool IsRunning() const;

private:
	void tick();
	bool createResources();
	void requestReadbacks();
	void onAlbedoReadback(const HTexturePixels& inPixels);
	void judge();
	void finish();
	void check(bool bCondition, const PString& inWhat);
	void writeEvidence() const;
};
