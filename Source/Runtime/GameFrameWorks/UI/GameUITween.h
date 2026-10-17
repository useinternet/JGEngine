#pragma once
#include "UI/GameUIDefines.h"

class PGameUIElement;
class PGameUIManager;
class PGameUITimeline;

// 게임 UI 움직임(GG-16, ProjectAH ER-017): 트윈 · 이징 · 순서 묶음(타임라인) · 위젯 열림 · 닫힘 전환.
// 이징과 묶음 진행(차례 · 동시 · 기다리기 · 함수 부르기 · 되풀이 · 건너뛰기)은 UI 타입에 기대지 않는다 — 요소 속성을 읽고 쓰는 곳만 UI 요소에 붙는다.
// 묶음은 위젯이 가진다(JGGameWidget::PlayTimeline · PlayTween). 관리자 Update 가 활성 위젯 · 닫히는 위젯의 묶음만 돌린다 —
// 다른 위젯에 덮이면 멈추고, 위젯이 화면에서 완전히 빠지면(OnShutdown 전에) 취소된다. 메인 스레드 전용.

// ---- 이징 (빠르기 곡선)
enum class EGameUIEase : int32
{
	Linear,
	InQuad,
	OutQuad,
	InOutQuad,
	InCubic,
	OutCubic,
	InOutCubic,
	InBack,       // 처음에 살짝 뒤로
	OutBack,      // 끝에서 살짝 넘었다가 돌아옴
	InOutBack,
	InElastic,    // 처음에 출렁임
	OutElastic,   // 끝에서 출렁임(튕기기)
	InOutElastic,
	Count,
};

// 게임이 주는 곡선: 진행 0~1 → 값. 0 에서 0, 1 에서 1 이 보통이고 사이에서 0~1 을 넘어도 된다(넘친 값만큼 끝값을 지나간다).
using FGameUIEaseCurve = float32 (*)(float32 t);

struct GAMEFRAMEWORKS_API HGameUIEase
{
	// t 는 0~1 로 자른다. Back · Elastic 은 결과가 0~1 을 넘는다.
	static float32 Evaluate(EGameUIEase ease, float32 t);
};

// ---- 트윈 값: 위치 · 크기 · 배율 · 옮기기 · 넘긴 양 = X · Y, 각도 · 불투명도 = X, 색 = X · Y · Z · W(R · G · B · A).
struct HGameUITweenValue
{
	float32 X = 0.0f;
	float32 Y = 0.0f;
	float32 Z = 0.0f;
	float32 W = 0.0f;

	HGameUITweenValue() = default;
	HGameUITweenValue(float32 value) : X(value) {}
	HGameUITweenValue(const HVector2& value) : X(value.x), Y(value.y) {}
	HGameUITweenValue(const HLinearColor& value) : X(value.R), Y(value.G), Z(value.B), W(value.A) {}

	static HGameUITweenValue Lerp(const HGameUITweenValue& a, const HGameUITweenValue& b, float32 t)
	{
		HGameUITweenValue result;
		result.X = a.X + (b.X - a.X) * t;
		result.Y = a.Y + (b.Y - a.Y) * t;
		result.Z = a.Z + (b.Z - a.Z) * t;
		result.W = a.W + (b.W - a.W) * t;
		return result;
	}
};

// ---- 트윈 하나: 대상 요소(약참조)의 속성 하나를 시작값 → 끝값으로, 길이(초) · 지연 · 이징.
// 시작값을 주지 않으면 이 트윈이 실제로 시작하는 순간(지연 · 앞 단계가 끝난 뒤)의 지금 값에서 시작한다(되풀이 · 왕복은 처음 읽은 값을 그대로 쓴다).
// 같은 요소 · 같은 속성에 다른 묶음의 트윈이 시작되면 그 속성을 쓰던 옛 묶음은 거기서 취소로 끝난다(Stop 과 같다 — 값은 그 자리).
// 같은 묶음 안의 단계끼리는 서로 막지 않는다(같은 프레임에 둘이 쓰면 뒤 단계 값).
// 대상이 사라지면(트리를 다시 짜기 · 위젯 닫힘) 그 묶음은 오류 없이 취소로 끝난다.
struct GAMEFRAMEWORKS_API HGameUITween
{
	PWeakPtr<PGameUIElement> Target;
	EGameUITweenProperty     Property = EGameUITweenProperty::Opacity;
	HGameUITweenValue        FromValue;
	HGameUITweenValue        ToValue;
	bool                     bHasFromValue = false;
	float32                  Duration = 0.0f;
	float32                  Delay    = 0.0f;
	EGameUIEase              Ease     = EGameUIEase::OutQuad;
	FGameUIEaseCurve         Curve    = nullptr;   // 있으면 Ease 대신

	static HGameUITween To(const PSharedPtr<PGameUIElement>& target, EGameUITweenProperty property, const HGameUITweenValue& to,
		float32 duration, EGameUIEase ease = EGameUIEase::OutQuad);
	static HGameUITween FromTo(const PSharedPtr<PGameUIElement>& target, EGameUITweenProperty property, const HGameUITweenValue& from,
		const HGameUITweenValue& to, float32 duration, EGameUIEase ease = EGameUIEase::OutQuad);
	HGameUITween& SetDelay(float32 delay);
	HGameUITween& SetCurve(FGameUIEaseCurve curve);
};

// 묶음 안에서 부르는 함수(차례 묶음이 그 자리에 오면 한 번).
JG_DECLARE_DELEGATE(HGameUITimelineCall);
// 묶음이 끝났다. 인자 = 취소됐나(Stop · 대상이 사라짐 · 같은 속성을 다른 묶음이 가져감 · 위젯이 빠짐 · 안의 자식 묶음을 밖에서 Stop). 끝까지 갔거나 Skip 이면 false. 한 번만 온다.
JG_DECLARE_EVENT(HOnGameUITimelineFinished, PGameUITimeline, bool);

// ---- 순서 묶음(타임라인). 차례(Sequence) 또는 동시(Parallel)로 트윈 · 기다리기 · 함수 · 다른 묶음을 담는다.
// 만든 뒤 위젯에서 재생한다: widget->PlayTimeline(timeline). 단계는 재생 전에만 넣는다.
//   PSharedPtr<PGameUITimeline> flight = PGameUITimeline::Sequence();
//   flight->Add(HGameUITween::To(card, EGameUITweenProperty::Translation, HVector2(0.0f, 0.0f), 0.3f, EGameUIEase::OutCubic))
//         .Call([this]() { onLanded(); })
//         .Add(HGameUITween::To(card, EGameUITweenProperty::Scale, HVector2(1.0f, 1.0f), 0.2f, EGameUIEase::OutBack));
//   PlayTimeline(flight);
class GAMEFRAMEWORKS_API PGameUITimeline : public IMemoryObject
{
	friend class JGGameWidget;
	friend class PGameUIManager;

public:
	HOnGameUITimelineFinished OnFinished;

	enum class EKind : uint8
	{
		Sequence,
		Parallel,
	};

private:
	enum class EStepType : uint8
	{
		Tween,
		Wait,
		Call,
		Child,
	};
	enum class EState : uint8
	{
		Idle,
		Playing,
		Finished,
	};
	struct HStep
	{
		EStepType                   Type = EStepType::Wait;
		HGameUITween                Tween;
		float32                     Seconds = 0.0f;
		HGameUITimelineCall         Callback;
		PSharedPtr<PGameUITimeline> Child;

		// 재생 상태(바퀴마다 처음으로)
		float32           Elapsed   = 0.0f;
		bool              bStarted  = false;
		bool              bDone     = false;
		bool              bCaptured = false;   // 시작값을 읽었나(재생 동안 — 되풀이 · 왕복에서 그대로 쓴다)
		HGameUITweenValue CapturedFrom;
	};

	EKind         _kind  = EKind::Sequence;
	HList<HStep>  _steps;
	int32         _loops = 1;        // 0 이하 = 끝없이
	bool          _bYoyo = false;

	EState          _state     = EState::Idle;
	int32           _iteration = 0;
	bool            _bReverse  = false;   // 이번 바퀴가 거꾸로(왕복)
	uint64          _cursor    = 0;       // 차례 묶음의 지금 단계(거꾸로면 뒤에서부터 센다)
	bool            _bCanceled = false;   // 끝 알림에 취소로
	bool            _bSkipping = false;   // Skip 중: 끝없는 되풀이도 이번 바퀴에서 멈춘다
	bool            _bChild    = false;   // 다른 묶음 안에서 돈다(쓰기 번호는 맨 위 묶음이 놓는다)
	uint64          _writerId  = 0;       // 이 재생이 요소 속성에 쓰는 번호(자식 묶음은 맨 위 묶음과 같다)
	PGameUIManager* _manager   = nullptr; // 재생하는 위젯의 관리자(글 색의 기본값을 읽을 때). 없어도 된다

public:
	PGameUITimeline() = default;
	virtual ~PGameUITimeline() = default;

	static PSharedPtr<PGameUITimeline> Sequence();
	static PSharedPtr<PGameUITimeline> Parallel();
	EKind GetKind() const;

	// ---- 단계(재생 전에만 — 재생 중이면 무시하고 경고)
	PGameUITimeline& Add(const HGameUITween& tween);
	PGameUITimeline& Add(const PSharedPtr<PGameUITimeline>& child);
	PGameUITimeline& Wait(float32 seconds);
	PGameUITimeline& Call(const HGameUITimelineCall& callback);
	// 람다를 바로 받는다(대리자 자체는 위 함수로).
	template<class TFunction, class = std::enable_if_t<std::is_same_v<std::decay_t<TFunction>, HGameUITimelineCall> == false>>
	PGameUITimeline& Call(TFunction&& function)
	{
		const HGameUITimelineCall callback = HGameUITimelineCall::CreateLambda(std::forward<TFunction>(function));
		return Call(callback);
	}
	// 되풀이: count = 모두 몇 바퀴(1 = 한 번, 0 이하 = 끝없이). bYoyo 면 짝수 번째 바퀴는 거꾸로(끝값 → 시작값, 단계도 뒤에서부터).
	PGameUITimeline& SetLoops(int32 count, bool bYoyo = false);

	// ---- 제어
	// 취소로 끝낸다(OnFinished(true)). 값은 지금 자리에 두고, 남은 단계 · 함수는 하지 않는다.
	void Stop();
	// 끝으로 건너뛴다: 남은 트윈은 끝값, 남은 기다리기는 넘기고, 남은 함수는 차례대로 부른다. OnFinished(false) 한 번(앞서 취소가 정해졌으면 true).
	// 끝없이 되풀이하는 묶음은 이번 바퀴 끝에서 멈춘다.
	void Skip();
	bool IsPlaying() const;
	bool IsFinished() const;
	// 한 바퀴 길이(초, 되풀이 제외). 동시 묶음은 가장 긴 단계.
	float32 GetDuration() const;

private:
	// 위젯이 재생을 시작한다: 처음 상태로 돌리고 0초만큼 진행(첫 트윈의 시작값을 바로 쓴다 — 첫 그리기가 시작값으로 나온다).
	void play(PGameUIManager* manager);
	// 부모 묶음이 이 묶음(자식 단계)에 들어올 때마다: 바퀴 상태만 처음으로(시작값은 맨 위 묶음의 play 가 지운다).
	void beginAsChild(PGameUIManager* manager, bool bSkipping, uint64 writerId);
	// 시작값 · 취소 표시를 지운다(자식 묶음까지).
	void resetCapture();
	// dt 만큼 진행. 끝났으면 true(끝 알림까지 보냄).
	bool tick(float32 deltaSeconds);
	// 남은 시간을 돌려준다: 0 이상 = 이 바퀴 · 묶음이 끝남, Running = 아직, Lost = 대상이 사라졌거나 같은 속성을 다른 묶음이 가져가 취소.
	float32 advance(float32 deltaSeconds);
	float32 advanceIteration(float32 deltaSeconds);
	float32 advanceStep(HStep& step, float32 deltaSeconds);
	void    resetIteration();
	void    finish(bool bCanceled);
	// 이 번호로 가져간 요소 속성을 놓는다(자식 묶음까지. 다른 묶음이 가져간 것은 그대로).
	void    releaseWriters(uint64 writerId);
	bool    isInfinite() const;
	// 요소 속성 쓰기(UI 요소에 붙는 부분). 넘긴 양은 스크롤 칸의 트윈 전용 경로로 쓴다(그 트윈을 스스로 취소하지 않게).
	static void writeProperty(PSharedPtr<PGameUIElement> element, EGameUITweenProperty property, const HGameUITweenValue& value);

	static constexpr float32 Running = -1.0f;
	static constexpr float32 Lost    = -2.0f;
};

// ---- 위젯 열림 · 닫힘 전환. 닫혀 있는 모습(불투명도 · 배율 · 옮기기)과 열린 모습(1 · 1 · 0) 사이를 위젯 루트 요소에서 움직인다.
// 길이 0 = 전환 없음(바로 나타나고 바로 사라진다 — 지금까지와 같음). 배율은 화면 가운데를 기준으로 한다.
struct GAMEFRAMEWORKS_API HGameUITransition
{
	float32     Duration = 0.0f;
	EGameUIEase Ease     = EGameUIEase::OutCubic;
	float32     Opacity  = 0.0f;                    // 닫혀 있는 모습
	HVector2    Scale    = HVector2(1.0f, 1.0f);
	HVector2    Offset;                             // 논리 단위

	static HGameUITransition Fade(float32 duration, EGameUIEase ease = EGameUIEase::OutCubic);
	// 흐려지며 + 배율(예: 0.92 에서 1 로 커지며 나타나기)
	static HGameUITransition FadeScale(float32 duration, float32 scale, EGameUIEase ease = EGameUIEase::OutCubic);
	// 흐려지며 + 옮기기(예: 아래 60 에서 올라오며 나타나기)
	static HGameUITransition FadeSlide(float32 duration, const HVector2& offset, EGameUIEase ease = EGameUIEase::OutCubic);
	bool IsEnabled() const;
};
