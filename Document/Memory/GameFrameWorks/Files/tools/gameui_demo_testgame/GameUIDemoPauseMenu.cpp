#include "PCH/PCH.h"
#include "GameUIDemoPauseMenu.h"
#include "UI/GameUI.h"

void JGGameUIDemoPauseMenu::OnInitialize()
{
	SetInputMode(EGameWidgetInputMode::Menu);
	SetBackHandler(true);

	// 화면 전체를 살짝 어둡게(입력 차단은 입력 모드가 한다 — 이 막은 모습만)
	PSharedPtr<PGameUIImage> dim = AddChild<PGameUIImage>(PName("Dim"));
	dim->SetLayout(HVector2(0.0f, 0.0f), HVector2(0.0f, 0.0f), HVector2(0.0f, 0.0f), HVector2(1920.0f, 1080.0f));
	dim->SetColor(HLinearColor(0.0f, 0.0f, 0.0f, 0.35f));

	// 가운데 패널
	PSharedPtr<PGameUIImage> panel = AddChild<PGameUIImage>(PName("Panel"));
	panel->SetLayout(HVector2(0.5f, 0.5f), HVector2(0.5f, 0.5f), HVector2(0.0f, 0.0f), HVector2(640.0f, 400.0f));
	panel->SetColor(HLinearColor(0.08f, 0.10f, 0.16f, 0.96f));

	PSharedPtr<PGameUIText> title = panel->AddChild<PGameUIText>(PName("Title"));
	title->SetLayout(HVector2(0.0f, 0.0f), HVector2(0.0f, 0.0f), HVector2(0.0f, 30.0f), HVector2(640.0f, 80.0f));
	title->SetFontSize(48.0f);
	title->SetAlignment(EGameUIHorizontalAlign::Center, EGameUIVerticalAlign::Middle);
	title->SetText(PString("일시정지 · Paused"));

	PSharedPtr<PGameUIText> help = panel->AddChild<PGameUIText>(PName("Help"));
	help->SetLayout(HVector2(0.0f, 0.0f), HVector2(0.0f, 0.0f), HVector2(0.0f, 120.0f), HVector2(640.0f, 60.0f));
	help->SetFontSize(26.0f);
	help->SetColor(HLinearColor(0.72f, 0.80f, 0.92f, 1.0f));
	help->SetAlignment(EGameUIHorizontalAlign::Center, EGameUIVerticalAlign::Middle);
	help->SetText(PString("Esc 또는 Resume 으로 닫기"));

	PSharedPtr<PGameUIButton> resume = panel->AddChild<PGameUIButton>(PName("Resume"));
	resume->SetLayout(HVector2(0.5f, 1.0f), HVector2(0.5f, 1.0f), HVector2(0.0f, -50.0f), HVector2(320.0f, 100.0f));
	resume->SetLabel(PString("Resume · 계속"));
	resume->OnClicked.AddLambda([this]()
		{
			JG_LOG(GameUIDemo, ELogLevel::Info, "GameUIDemo Resume clicked");
			DeactivateWidget();
		});
}

void JGGameUIDemoPauseMenu::OnActivated()
{
	JG_LOG(GameUIDemo, ELogLevel::Info, "GameUIDemo pause menu activated");
}

void JGGameUIDemoPauseMenu::OnShutdown()
{
	JG_LOG(GameUIDemo, ELogLevel::Info, "GameUIDemo pause menu closed");
}
