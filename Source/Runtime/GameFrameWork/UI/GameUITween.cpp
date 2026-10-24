#include "PCH/PCH.h"
#include "UI/GameUITween.h"
#include "UI/GameUIElement.h"
#include "UI/GameUIImage.h"
#include "UI/GameUIText.h"
#include "UI/GameUIScrollBox.h"
#include "UI/GameUIManager.h"
#include <cmath>

namespace
{
	constexpr float32 Pi          = 3.14159265358979f;
	constexpr float32 BackC1      = 1.70158f;
	constexpr float32 BackC2      = BackC1 * 1.525f;
	constexpr float32 BackC3      = BackC1 + 1.0f;
	constexpr float32 ElasticC4   = (2.0f * Pi) / 3.0f;
	constexpr float32 ElasticC5   = (2.0f * Pi) / 4.5f;
	constexpr float32 SkipSeconds = 1.0e6f;   // Skip: 남은 시간을 한꺼번에

	// 같은 요소 · 같은 속성을 쓰는 묶음(재생 한 번)을 가르는 번호(메인 스레드 전용).
	uint64 GNextTweenWriterId = 0;

	// ---- 요소 속성 읽기(UI 요소에 붙는 부분. 묶음 진행은 이것과 PGameUITimeline::writeProperty 만 거친다)
	HGameUITweenValue readProperty(const PSharedPtr<PGameUIElement>& element, EGameUITweenProperty property, const PGameUIManager* manager)
	{
		switch (property)
		{
		case EGameUITweenProperty::Position:
			return HGameUITweenValue(element->GetPosition());
		case EGameUITweenProperty::Size:
			return HGameUITweenValue(element->GetSize());
		case EGameUITweenProperty::Angle:
			return HGameUITweenValue(element->GetRenderTransform().Angle);
		case EGameUITweenProperty::Scale:
			return HGameUITweenValue(element->GetRenderTransform().Scale);
		case EGameUITweenProperty::Translation:
			return HGameUITweenValue(element->GetRenderTransform().Translation);
		case EGameUITweenProperty::Opacity:
			return HGameUITweenValue(element->GetOpacity());
		case EGameUITweenProperty::Color:
		{
			// PSharedPtr 는 const 를 전파하므로 비 const 사본으로 바꾼다.
			PSharedPtr<PGameUIElement> candidate = element;
			PSharedPtr<PGameUIImage> image = RawDynamicCast<PGameUIImage>(candidate);
			if (image.IsValid())
			{
				return HGameUITweenValue(image->GetColor());
			}
			PSharedPtr<PGameUIText> text = RawDynamicCast<PGameUIText>(candidate);
			if (text.IsValid())
			{
				return HGameUITweenValue(text->GetColor(manager));
			}
			return HGameUITweenValue(HLinearColor(1.0f, 1.0f, 1.0f, 1.0f));
		}
		case EGameUITweenProperty::ScrollOffset:
		{
			PSharedPtr<PGameUIElement> candidate = element;
			PSharedPtr<PGameUIScrollBox> box = RawDynamicCast<PGameUIScrollBox>(candidate);
			return box.IsValid() ? HGameUITweenValue(box->GetScrollOffset()) : HGameUITweenValue();
		}
		default:
			break;
		}
		return HGameUITweenValue();
	}
}

// ---- 요소 속성 쓰기(UI 요소에 붙는 부분)
void PGameUITimeline::writeProperty(PSharedPtr<PGameUIElement> element, EGameUITweenProperty property, const HGameUITweenValue& value)
{
	switch (property)
	{
	case EGameUITweenProperty::Position:
		element->SetPosition(HVector2(value.X, value.Y));
		break;
	case EGameUITweenProperty::Size:
		element->SetSize(HVector2(value.X, value.Y));
		break;
	case EGameUITweenProperty::Angle:
	{
		HGameUIRenderTransform transform = element->GetRenderTransform();
		transform.Angle = value.X;
		element->SetRenderTransform(transform);
		break;
	}
	case EGameUITweenProperty::Scale:
	{
		HGameUIRenderTransform transform = element->GetRenderTransform();
		transform.Scale = HVector2(value.X, value.Y);
		element->SetRenderTransform(transform);
		break;
	}
	case EGameUITweenProperty::Translation:
	{
		HGameUIRenderTransform transform = element->GetRenderTransform();
		transform.Translation = HVector2(value.X, value.Y);
		element->SetRenderTransform(transform);
		break;
	}
	case EGameUITweenProperty::Opacity:
		element->SetOpacity(value.X);
		break;
	case EGameUITweenProperty::Color:
	{
		const HLinearColor color(value.X, value.Y, value.Z, value.W);
		PSharedPtr<PGameUIImage> image = RawDynamicCast<PGameUIImage>(element);
		if (image.IsValid())
		{
			image->SetColor(color);
			break;
		}
		PSharedPtr<PGameUIText> text = RawDynamicCast<PGameUIText>(element);
		if (text.IsValid())
		{
			text->SetColor(color);
		}
		// 그 밖의 요소(버튼 · 입력란 — 색이 스타일에 있다)는 움직이지 않는다.
		break;
	}
	case EGameUITweenProperty::ScrollOffset:
	{
		// 스크롤 칸만. ScrollTo 와 달리 넘긴 양 트윈(이 묶음)을 취소하지 않는다.
		PSharedPtr<PGameUIScrollBox> box = RawDynamicCast<PGameUIScrollBox>(element);
		if (box.IsValid())
		{
			box->setOffsetFromTween(HVector2(value.X, value.Y));
		}
		break;
	}
	default:
		break;
	}
}

// ---- 이징 (식은 easings.net 의 Robert Penner 식)
float32 HGameUIEase::Evaluate(EGameUIEase ease, float32 t)
{
	t = HMath::Clamp(t, 0.0f, 1.0f);
	switch (ease)
	{
	case EGameUIEase::Linear:
		return t;
	case EGameUIEase::InQuad:
		return t * t;
	case EGameUIEase::OutQuad:
		return 1.0f - (1.0f - t) * (1.0f - t);
	case EGameUIEase::InOutQuad:
		return (t < 0.5f) ? 2.0f * t * t : 1.0f - std::pow(-2.0f * t + 2.0f, 2.0f) * 0.5f;
	case EGameUIEase::InCubic:
		return t * t * t;
	case EGameUIEase::OutCubic:
		return 1.0f - std::pow(1.0f - t, 3.0f);
	case EGameUIEase::InOutCubic:
		return (t < 0.5f) ? 4.0f * t * t * t : 1.0f - std::pow(-2.0f * t + 2.0f, 3.0f) * 0.5f;
	case EGameUIEase::InBack:
		return BackC3 * t * t * t - BackC1 * t * t;
	case EGameUIEase::OutBack:
		return 1.0f + BackC3 * std::pow(t - 1.0f, 3.0f) + BackC1 * std::pow(t - 1.0f, 2.0f);
	case EGameUIEase::InOutBack:
		if (t < 0.5f)
		{
			return (std::pow(2.0f * t, 2.0f) * ((BackC2 + 1.0f) * 2.0f * t - BackC2)) * 0.5f;
		}
		return (std::pow(2.0f * t - 2.0f, 2.0f) * ((BackC2 + 1.0f) * (t * 2.0f - 2.0f) + BackC2) + 2.0f) * 0.5f;
	case EGameUIEase::InElastic:
		if (t <= 0.0f || t >= 1.0f)
		{
			return t;
		}
		return -std::pow(2.0f, 10.0f * t - 10.0f) * std::sin((t * 10.0f - 10.75f) * ElasticC4);
	case EGameUIEase::OutElastic:
		if (t <= 0.0f || t >= 1.0f)
		{
			return t;
		}
		return std::pow(2.0f, -10.0f * t) * std::sin((t * 10.0f - 0.75f) * ElasticC4) + 1.0f;
	case EGameUIEase::InOutElastic:
		if (t <= 0.0f || t >= 1.0f)
		{
			return t;
		}
		if (t < 0.5f)
		{
			return -(std::pow(2.0f, 20.0f * t - 10.0f) * std::sin((20.0f * t - 11.125f) * ElasticC5)) * 0.5f;
		}
		return (std::pow(2.0f, -20.0f * t + 10.0f) * std::sin((20.0f * t - 11.125f) * ElasticC5)) * 0.5f + 1.0f;
	default:
		break;
	}
	return t;
}

// ---- 트윈 하나
HGameUITween HGameUITween::To(const PSharedPtr<PGameUIElement>& target, EGameUITweenProperty property, const HGameUITweenValue& to, float32 duration, EGameUIEase ease)
{
	HGameUITween tween;
	tween.Target   = target;
	tween.Property = property;
	tween.ToValue  = to;
	tween.Duration = duration;
	tween.Ease     = ease;
	return tween;
}

HGameUITween HGameUITween::FromTo(const PSharedPtr<PGameUIElement>& target, EGameUITweenProperty property, const HGameUITweenValue& from, const HGameUITweenValue& to,
	float32 duration, EGameUIEase ease)
{
	HGameUITween tween = To(target, property, to, duration, ease);
	tween.FromValue     = from;
	tween.bHasFromValue = true;
	return tween;
}

HGameUITween& HGameUITween::SetDelay(float32 delay)
{
	Delay = delay;
	return *this;
}

HGameUITween& HGameUITween::SetCurve(FGameUIEaseCurve curve)
{
	Curve = curve;
	return *this;
}

// ---- 순서 묶음
PSharedPtr<PGameUITimeline> PGameUITimeline::Sequence()
{
	PSharedPtr<PGameUITimeline> timeline = Allocate<PGameUITimeline>();
	timeline->_kind = EKind::Sequence;
	return timeline;
}

PSharedPtr<PGameUITimeline> PGameUITimeline::Parallel()
{
	PSharedPtr<PGameUITimeline> timeline = Allocate<PGameUITimeline>();
	timeline->_kind = EKind::Parallel;
	return timeline;
}

PGameUITimeline::EKind PGameUITimeline::GetKind() const
{
	return _kind;
}

PGameUITimeline& PGameUITimeline::Add(const HGameUITween& tween)
{
	if (_state == EState::Playing)
	{
		JG_LOG(GameUI, ELogLevel::Warning, "Timeline: steps can be added only before it plays");
		return *this;
	}
	HStep step;
	step.Type           = EStepType::Tween;
	step.Tween          = tween;
	step.Tween.Duration = HMath::Max(tween.Duration, 0.0f);
	step.Tween.Delay    = HMath::Max(tween.Delay, 0.0f);
	_steps.push_back(step);
	return *this;
}

PGameUITimeline& PGameUITimeline::Add(const PSharedPtr<PGameUITimeline>& child)
{
	if (_state == EState::Playing)
	{
		JG_LOG(GameUI, ELogLevel::Warning, "Timeline: steps can be added only before it plays");
		return *this;
	}
	if (child.IsValid() == false || child.GetRawPointer() == this)
	{
		return *this;
	}
	HStep step;
	step.Type  = EStepType::Child;
	step.Child = child;
	_steps.push_back(step);
	return *this;
}

PGameUITimeline& PGameUITimeline::Wait(float32 seconds)
{
	if (_state == EState::Playing)
	{
		JG_LOG(GameUI, ELogLevel::Warning, "Timeline: steps can be added only before it plays");
		return *this;
	}
	HStep step;
	step.Type    = EStepType::Wait;
	step.Seconds = HMath::Max(seconds, 0.0f);
	_steps.push_back(step);
	return *this;
}

PGameUITimeline& PGameUITimeline::Call(const HGameUITimelineCall& callback)
{
	if (_state == EState::Playing)
	{
		JG_LOG(GameUI, ELogLevel::Warning, "Timeline: steps can be added only before it plays");
		return *this;
	}
	HStep step;
	step.Type     = EStepType::Call;
	step.Callback = callback;
	_steps.push_back(step);
	return *this;
}

PGameUITimeline& PGameUITimeline::SetLoops(int32 count, bool bYoyo)
{
	_loops = count;
	_bYoyo = bYoyo;
	return *this;
}

void PGameUITimeline::Stop()
{
	if (_state != EState::Playing)
	{
		return;
	}
	_bCanceled = true;
	finish(true);
}

void PGameUITimeline::Skip()
{
	if (_state != EState::Playing)
	{
		return;
	}
	// 남은 시간을 한꺼번에 준다: 트윈은 끝값, 기다리기는 넘기고, 함수는 차례대로. 끝없는 되풀이는 이번 바퀴 끝에서 멈춘다.
	_bSkipping = true;
	for (int32 guard = 0; guard < 100000 && _state == EState::Playing; ++guard)
	{
		tick(SkipSeconds);
	}
}

bool PGameUITimeline::IsPlaying() const
{
	return _state == EState::Playing;
}

bool PGameUITimeline::IsFinished() const
{
	return _state == EState::Finished;
}

float32 PGameUITimeline::GetDuration() const
{
	float32 total = 0.0f;
	for (const HStep& step : _steps)
	{
		float32 length = 0.0f;
		switch (step.Type)
		{
		case EStepType::Tween:
			length = step.Tween.Delay + step.Tween.Duration;
			break;
		case EStepType::Wait:
			length = step.Seconds;
			break;
		case EStepType::Child:
			length = step.Child->GetDuration() * (float32)HMath::Max(step.Child->_loops, 1);
			break;
		default:
			break;
		}
		total = (_kind == EKind::Sequence) ? total + length : HMath::Max(total, length);
	}
	return total;
}

void PGameUITimeline::play(PGameUIManager* manager)
{
	if (_state == EState::Playing)
	{
		return;
	}
	resetCapture();
	_manager   = manager;
	_writerId  = ++GNextTweenWriterId;
	_bChild    = false;
	_state     = EState::Playing;
	_iteration = 0;
	_bReverse  = false;
	_bSkipping = false;
	resetIteration();
	// 0초만큼 진행해 첫 트윈의 시작값을 바로 쓴다(재생을 건 프레임의 그리기가 시작값으로 나온다).
	tick(0.0f);
}

void PGameUITimeline::beginAsChild(PGameUIManager* manager, bool bSkipping, uint64 writerId)
{
	_manager   = manager;
	_writerId  = writerId;
	_bChild    = true;
	_state     = EState::Playing;
	_iteration = 0;
	_bReverse  = false;
	_bSkipping = bSkipping;
	resetIteration();
}

void PGameUITimeline::resetCapture()
{
	_bCanceled = false;
	for (HStep& step : _steps)
	{
		step.bCaptured = false;
		if (step.Type == EStepType::Child && step.Child.IsValid())
		{
			step.Child->resetCapture();
		}
	}
}

bool PGameUITimeline::tick(float32 deltaSeconds)
{
	if (_state != EState::Playing)
	{
		return _state == EState::Finished;
	}

	const float32 left = advance(deltaSeconds);
	if (_state != EState::Playing)
	{
		// 진행 중에 함수 · 알림이 Stop 등으로 이미 끝냈다.
		return true;
	}
	if (left == Lost)
	{
		finish(true);
		return true;
	}
	if (left >= 0.0f)
	{
		finish(_bCanceled);
		return true;
	}
	return false;
}

float32 PGameUITimeline::advance(float32 deltaSeconds)
{
	float32 remaining = deltaSeconds;
	while (true)
	{
		const float32 left = advanceIteration(remaining);
		if (left < 0.0f || _state != EState::Playing)
		{
			return (_state != EState::Playing) ? Running : left;
		}

		++_iteration;
		if ((isInfinite() == false && _iteration >= _loops) || (isInfinite() && _bSkipping))
		{
			return left;
		}

		// 다음 바퀴. 이번 바퀴가 시간을 쓰지 않았으면(길이 0) 다음 진행에서 이어 간다 — 길이 0 묶음을 끝없이 되풀이해도 한 진행에 멈추지 않게.
		const bool bConsumedTime = left < remaining;
		if (_bYoyo)
		{
			_bReverse = !_bReverse;
		}
		resetIteration();
		remaining = left;
		if (bConsumedTime == false && _bSkipping == false)
		{
			return Running;
		}
	}
}

float32 PGameUITimeline::advanceIteration(float32 deltaSeconds)
{
	if (_kind == EKind::Sequence)
	{
		float32 remaining = deltaSeconds;
		while (_cursor < (uint64)_steps.size())
		{
			const uint64 index = _bReverse ? ((uint64)_steps.size() - 1 - _cursor) : _cursor;
			const float32 left = advanceStep(_steps[(size_t)index], remaining);
			if (left < 0.0f || _state != EState::Playing)
			{
				return left;
			}
			remaining = left;
			++_cursor;
		}
		return remaining;
	}

	// 동시: 모든 단계가 같은 시간을 받고, 가장 늦게 끝나는 단계와 함께 끝난다.
	float32 smallestLeft = deltaSeconds;
	bool    bAllDone     = true;
	for (size_t index = 0; index < _steps.size(); ++index)
	{
		HStep& step = _steps[index];
		if (step.bDone)
		{
			continue;
		}
		const float32 left = advanceStep(step, deltaSeconds);
		if (left == Lost || _state != EState::Playing)
		{
			return left;
		}
		if (left < 0.0f)
		{
			bAllDone = false;
			continue;
		}
		smallestLeft = HMath::Min(smallestLeft, left);
	}
	return bAllDone ? smallestLeft : Running;
}

float32 PGameUITimeline::advanceStep(HStep& step, float32 deltaSeconds)
{
	if (step.bDone)
	{
		return deltaSeconds;
	}

	switch (step.Type)
	{
	case EStepType::Wait:
	{
		step.Elapsed += deltaSeconds;
		if (step.Elapsed < step.Seconds)
		{
			return Running;
		}
		step.bDone = true;
		return step.Elapsed - step.Seconds;
	}
	case EStepType::Call:
	{
		step.bDone = true;
		step.Callback.ExecuteIfBound();
		return deltaSeconds;
	}
	case EStepType::Child:
	{
		PSharedPtr<PGameUITimeline> child = step.Child;
		if (step.bStarted == false)
		{
			step.bStarted = true;
			child->beginAsChild(_manager, _bSkipping, _writerId);
		}
		else if (child->_state == EState::Finished)
		{
			// 자식이 밖에서 멈췄다(Stop): 이 단계는 끝, 묶음 끝 알림은 취소로.
			step.bDone = true;
			_bCanceled = true;
			return deltaSeconds;
		}
		child->_manager   = _manager;
		child->_bSkipping = _bSkipping;
		const float32 left = child->advance(deltaSeconds);
		if (left == Lost)
		{
			return Lost;
		}
		if (left < 0.0f)
		{
			return Running;
		}
		step.bDone = true;
		if (child->_bCanceled)
		{
			_bCanceled = true;
		}
		child->finish(child->_bCanceled);
		return left;
	}
	case EStepType::Tween:
		break;
	default:
		return deltaSeconds;
	}

	HGameUITween& tween = step.Tween;
	step.Elapsed += deltaSeconds;
	if (step.Elapsed < tween.Delay)
	{
		return Running;
	}

	PSharedPtr<PGameUIElement> target = tween.Target.Pin();
	if (target.IsValid() == false)
	{
		// 대상이 사라졌다(트리를 다시 짬 · 위젯이 닫힘): 묶음을 오류 없이 취소로 끝낸다.
		return Lost;
	}

	uint64& writer = target->_tweenWriters[(int32)tween.Property];
	if (step.bStarted == false)
	{
		// 시작: 이 속성을 가져온다(앞서 쓰던 다른 묶음은 다음 진행에서 취소로 끝난다). 처음 시작이면 시작값을 읽는다(없으면 지금 값).
		step.bStarted = true;
		writer        = _writerId;
		if (step.bCaptured == false)
		{
			step.bCaptured    = true;
			step.CapturedFrom = tween.bHasFromValue ? tween.FromValue : readProperty(target, tween.Property, _manager);
		}
	}
	else if (writer != _writerId)
	{
		// 같은 속성을 다른 묶음이 가져갔다: 이 묶음은 여기서 취소로 끝난다(값은 그 자리, 남은 단계 · 함수는 하지 않는다).
		return Lost;
	}

	const float32 active   = step.Elapsed - tween.Delay;
	const float32 progress = (tween.Duration > 0.0f) ? HMath::Min(active / tween.Duration, 1.0f) : 1.0f;
	const float32 t        = _bReverse ? 1.0f - progress : progress;
	const float32 eased    = (tween.Curve != nullptr) ? tween.Curve(t) : HGameUIEase::Evaluate(tween.Ease, t);
	writeProperty(target, tween.Property, HGameUITweenValue::Lerp(step.CapturedFrom, tween.ToValue, eased));
	if (progress < 1.0f)
	{
		return Running;
	}

	step.bDone = true;
	return HMath::Max(active - tween.Duration, 0.0f);
}

void PGameUITimeline::resetIteration()
{
	_cursor = 0;
	for (HStep& step : _steps)
	{
		step.Elapsed  = 0.0f;
		step.bStarted = false;
		step.bDone    = false;
	}
}

void PGameUITimeline::finish(bool bCanceled)
{
	if (_state == EState::Finished)
	{
		return;
	}
	_state = EState::Finished;

	// 재생 중인 자식도 함께 끝낸다(취소).
	for (HStep& step : _steps)
	{
		if (step.Type == EStepType::Child && step.Child.IsValid() && step.Child->IsPlaying())
		{
			step.Child->finish(true);
		}
	}
	// 가져간 속성은 맨 위 묶음이 끝날 때 한 번에 놓는다(먼저 끝난 자식이 놓으면 같은 번호로 쓰는 형제 단계가 빼앗긴 것으로 보인다).
	if (_bChild == false)
	{
		releaseWriters(_writerId);
	}

	// 처리기 안에서 이 묶음의 마지막 참조가 풀려도 알림을 끝까지 보내도록 잡아 둔다.
	PSharedPtr<PGameUITimeline> keepAlive = SharedWrap(this);
	OnFinished.BroadCast(bCanceled);
}

void PGameUITimeline::releaseWriters(uint64 writerId)
{
	if (writerId == 0)
	{
		return;
	}
	for (HStep& step : _steps)
	{
		if (step.Type == EStepType::Child && step.Child.IsValid())
		{
			step.Child->releaseWriters(writerId);
			continue;
		}
		if (step.Type != EStepType::Tween)
		{
			continue;
		}
		PSharedPtr<PGameUIElement> target = step.Tween.Target.Pin();
		if (target.IsValid() && target->_tweenWriters[(int32)step.Tween.Property] == writerId)
		{
			target->_tweenWriters[(int32)step.Tween.Property] = 0;
		}
	}
}

bool PGameUITimeline::isInfinite() const
{
	return _loops <= 0;
}

// ---- 위젯 열림 · 닫힘 전환
HGameUITransition HGameUITransition::Fade(float32 duration, EGameUIEase ease)
{
	HGameUITransition transition;
	transition.Duration = duration;
	transition.Ease     = ease;
	transition.Opacity  = 0.0f;
	return transition;
}

HGameUITransition HGameUITransition::FadeScale(float32 duration, float32 scale, EGameUIEase ease)
{
	HGameUITransition transition = Fade(duration, ease);
	transition.Scale = HVector2(scale, scale);
	return transition;
}

HGameUITransition HGameUITransition::FadeSlide(float32 duration, const HVector2& offset, EGameUIEase ease)
{
	HGameUITransition transition = Fade(duration, ease);
	transition.Offset = offset;
	return transition;
}

bool HGameUITransition::IsEnabled() const
{
	return Duration > 0.0f;
}
