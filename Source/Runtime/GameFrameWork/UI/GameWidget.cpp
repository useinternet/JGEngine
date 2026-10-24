#include "PCH/PCH.h"
#include "UI/GameWidget.h"
#include "UI/GameUIManager.h"

JGGameWidget::JGGameWidget()
{
	_root = Allocate<PGameUIElement>();
	_root->_name = PName("Root");
}

PSharedPtr<PGameUIElement> JGGameWidget::GetRoot() const
{
	return _root;
}

PSharedPtr<PGameUIElement> JGGameWidget::FindElement(const PName& name) const
{
	return _root->FindChild(name, true);
}

bool JGGameWidget::IsInStack() const
{
	return _manager != nullptr && _bClosing == false;
}

bool JGGameWidget::IsActive() const
{
	return _bActive;
}

EGameUILayer JGGameWidget::GetLayer() const
{
	return _layer;
}

PGameUIManager* JGGameWidget::GetManager() const
{
	return _manager;
}

void JGGameWidget::DeactivateWidget()
{
	if (_manager == nullptr)
	{
		return;
	}
	// 관리자가 스택에서 빼는 동안 자기가 풀리지 않게 강한 참조로 넘긴다.
	_manager->RemoveWidget(SharedWrap(this));
}

void JGGameWidget::SetInputMode(EGameWidgetInputMode inputMode)
{
	_inputMode = inputMode;
}

EGameWidgetInputMode JGGameWidget::GetInputMode() const
{
	return _inputMode;
}

void JGGameWidget::SetBackHandler(bool bBackHandler)
{
	_bBackHandler = bBackHandler;
}

bool JGGameWidget::IsBackHandler() const
{
	return _bBackHandler;
}

bool JGGameWidget::OnBackAction()
{
	DeactivateWidget();
	return true;
}

PSharedPtr<PGameUITimeline> JGGameWidget::PlayTimeline(const PSharedPtr<PGameUITimeline>& timeline)
{
	if (timeline.IsValid() == false)
	{
		return nullptr;
	}
	// PSharedPtr 는 const 를 전파하므로 비 const 사본으로 재생한다.
	PSharedPtr<PGameUITimeline> playing = timeline;
	if (playing->IsPlaying())
	{
		return playing;
	}
	_timelines.push_back(playing);
	playing->play(_manager);
	return playing;
}

PSharedPtr<PGameUITimeline> JGGameWidget::PlayTween(const HGameUITween& tween)
{
	PSharedPtr<PGameUITimeline> timeline = PGameUITimeline::Sequence();
	timeline->Add(tween);
	return PlayTimeline(timeline);
}

void JGGameWidget::SetOpenTransition(const HGameUITransition& transition)
{
	_openTransition = transition;
}

void JGGameWidget::SetCloseTransition(const HGameUITransition& transition)
{
	_closeTransition = transition;
}

const HGameUITransition& JGGameWidget::GetOpenTransition() const
{
	return _openTransition;
}

const HGameUITransition& JGGameWidget::GetCloseTransition() const
{
	return _closeTransition;
}

bool JGGameWidget::IsClosing() const
{
	return _bClosing;
}

void JGGameWidget::tickTimelines(float32 deltaSeconds)
{
	// 진행 중에 함수 · 끝 알림이 새 묶음을 재생할 수 있으므로 사본으로 돈다(새 묶음은 이미 0초만큼 진행했다 — 다음 진행부터).
	const HList<PSharedPtr<PGameUITimeline>> timelines = _timelines;
	for (const PSharedPtr<PGameUITimeline>& candidate : timelines)
	{
		PSharedPtr<PGameUITimeline> timeline = candidate;
		if (timeline->IsPlaying())
		{
			// 스택에 올리기 전에 재생을 건 묶음도 지금 관리자를 본다.
			timeline->_manager = _manager;
			timeline->tick(deltaSeconds);
		}
	}

	for (auto it = _timelines.begin(); it != _timelines.end();)
	{
		if ((*it)->IsPlaying() == false)
		{
			it = _timelines.erase(it);
		}
		else
		{
			++it;
		}
	}
}

void JGGameWidget::cancelTimelines()
{
	// 취소 알림 처리기가 묶음을 더 걸 수 있으므로 비울 때까지 돈다(한 번 더 걸린 것도 취소).
	for (int32 guard = 0; guard < 8 && _timelines.empty() == false; ++guard)
	{
		const HList<PSharedPtr<PGameUITimeline>> timelines = _timelines;
		_timelines.clear();
		for (const PSharedPtr<PGameUITimeline>& candidate : timelines)
		{
			PSharedPtr<PGameUITimeline> timeline = candidate;
			timeline->Stop();
		}
	}
	_timelines.clear();
	_transition = nullptr;
}

void JGGameWidget::beginTransition(bool bOpen, bool bFromCurrent)
{
	// 도는 전환은 멈춘다(열림 중 닫기 · 닫힘 중 다시 열기 — 새 전환은 지금 모습에서 이어 간다).
	if (_transition.IsValid())
	{
		PSharedPtr<PGameUITimeline> previous = _transition;
		_transition = nullptr;
		previous->Stop();
	}

	const HGameUITransition& transition = bOpen ? _openTransition : _closeTransition;
	if (transition.IsEnabled() == false)
	{
		if (bOpen && bFromCurrent)
		{
			// 닫히다 다시 여는데 열림 전환이 없다: 바로 열린 모습으로.
			HGameUIRenderTransform look = _root->GetRenderTransform();
			look.Scale       = HVector2(1.0f, 1.0f);
			look.Translation = HVector2(0.0f, 0.0f);
			_root->SetRenderTransform(look);
			_root->SetOpacity(1.0f);
		}
		return;
	}

	// 루트의 닫힌 모습 ↔ 열린 모습. 바뀌는 속성만 움직인다.
	PSharedPtr<PGameUITimeline> timeline = PGameUITimeline::Parallel();
	auto addTrack = [&](EGameUITweenProperty property, const HGameUITweenValue& closedValue, const HGameUITweenValue& openValue)
	{
		if (bOpen && bFromCurrent == false)
		{
			timeline->Add(HGameUITween::FromTo(_root, property, closedValue, openValue, transition.Duration, transition.Ease));
		}
		else
		{
			timeline->Add(HGameUITween::To(_root, property, bOpen ? openValue : closedValue, transition.Duration, transition.Ease));
		}
	};
	if (transition.Opacity != 1.0f)
	{
		addTrack(EGameUITweenProperty::Opacity, HGameUITweenValue(transition.Opacity), HGameUITweenValue(1.0f));
	}
	if (transition.Scale.x != 1.0f || transition.Scale.y != 1.0f)
	{
		addTrack(EGameUITweenProperty::Scale, HGameUITweenValue(transition.Scale), HGameUITweenValue(HVector2(1.0f, 1.0f)));
	}
	if (transition.Offset.x != 0.0f || transition.Offset.y != 0.0f)
	{
		addTrack(EGameUITweenProperty::Translation, HGameUITweenValue(transition.Offset), HGameUITweenValue(HVector2(0.0f, 0.0f)));
	}
	_transition = PlayTimeline(timeline);
}

bool JGGameWidget::isTransitionPlaying() const
{
	return _transition.IsValid() && _transition->IsPlaying();
}
