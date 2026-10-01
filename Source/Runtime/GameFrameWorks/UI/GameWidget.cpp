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
	return _manager != nullptr;
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
