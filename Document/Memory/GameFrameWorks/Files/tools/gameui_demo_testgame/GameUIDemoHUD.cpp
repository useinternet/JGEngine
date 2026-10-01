#include "PCH/PCH.h"
#include "GameUIDemoHUD.h"
#include "GameUIDemoPauseMenu.h"
#include "UI/GameUI.h"

namespace
{
	// End Turn 버튼 색은 캡처에서 찾기 쉽게 주황(검증 스크립트가 이 색으로 버튼 위치를 찾는다).
	HGameUIButtonStyle makeEndTurnStyle()
	{
		HGameUIButtonStyle style;
		style.Normal         = HLinearColor(0.95f, 0.45f, 0.10f, 1.0f);
		style.Hovered        = HLinearColor(1.00f, 0.62f, 0.25f, 1.0f);
		style.Pressed        = HLinearColor(0.65f, 0.28f, 0.05f, 1.0f);
		style.Label.FontSize = 36.0f;
		return style;
	}
}

int32 JGGameUIDemoHUD::GetTurn() const
{
	return _turn;
}

void JGGameUIDemoHUD::OnInitialize()
{
	PSharedPtr<PGameUIManager> ui = JGGameInstance::Get().GetUI();

	// 위 띠: 반투명 패널. 입력을 받아 그 아래 월드 클릭을 막는다.
	PSharedPtr<PGameUIImage> topBar = AddChild<PGameUIImage>(PName("TopBar"));
	topBar->SetLayout(HVector2(0.0f, 0.0f), HVector2(0.0f, 0.0f), HVector2(0.0f, 0.0f), HVector2(1920.0f, 96.0f));
	topBar->SetColor(HLinearColor(0.02f, 0.03f, 0.05f, 0.72f));
	topBar->SetHitTestVisible(true);

	_turnText = topBar->AddChild<PGameUIText>(PName("TurnText"));
	_turnText->SetLayout(HVector2(0.0f, 0.0f), HVector2(0.0f, 0.0f), HVector2(40.0f, 0.0f), HVector2(900.0f, 96.0f));
	_turnText->SetFontSize(44.0f);
	_turnText->SetAlignment(EGameUIHorizontalAlign::Left, EGameUIVerticalAlign::Middle);
	updateTurnText();

	PSharedPtr<PGameUIText> hint = topBar->AddChild<PGameUIText>(PName("Hint"));
	hint->SetLayout(HVector2(1.0f, 0.0f), HVector2(1.0f, 0.0f), HVector2(-300.0f, 0.0f), HVector2(760.0f, 96.0f));
	hint->SetFontSize(28.0f);
	hint->SetColor(HLinearColor(0.72f, 0.80f, 0.92f, 1.0f));
	hint->SetAlignment(EGameUIHorizontalAlign::Right, EGameUIVerticalAlign::Middle);
	hint->SetText(PString("JGGameWidget (GameFrameWorks)\n게임 전용 UI · CommonUI식"));

	// Menu 버튼: 관리자 기본 버튼 스타일(정하지 않음)
	PSharedPtr<PGameUIButton> menu = topBar->AddChild<PGameUIButton>(PName("Menu"));
	menu->SetLayout(HVector2(1.0f, 0.0f), HVector2(1.0f, 0.0f), HVector2(-40.0f, 16.0f), HVector2(220.0f, 64.0f));
	menu->SetLabel(PString("Menu · 메뉴"));
	menu->OnClicked.AddLambda([this]()
		{
			onMenuClicked();
		});

	// 손패: 엔진 Content 의 카드 뒷면 두 장
	const char* const cardFiles[2] = { "RawResources/CardBack_Tree_Dark_63x88mm_300dpi.png", "RawResources/CardBack_Tree_Gold_63x88mm_300dpi.png" };
	for (int32 i = 0; i < 2; ++i)
	{
		PString cardPath;
		HFileHelper::CombinePath(HFileHelper::EngineContentDirectory(), PString(cardFiles[i]), &cardPath);

		PSharedPtr<PGameUIImage> card = AddChild<PGameUIImage>(PName(PString::Format("Card%d", i)));
		card->SetLayout(HVector2(0.0f, 1.0f), HVector2(0.0f, 1.0f), HVector2(40.0f + (float32)i * 200.0f, -40.0f), HVector2(180.0f, 251.0f));
		card->SetTexture(ui->LoadTexture(cardPath));
		card->SetHitTestVisible(true);
	}

	PSharedPtr<PGameUIText> handLabel = AddChild<PGameUIText>(PName("HandLabel"));
	handLabel->SetLayout(HVector2(0.0f, 1.0f), HVector2(0.0f, 1.0f), HVector2(40.0f, -300.0f), HVector2(600.0f, 48.0f));
	handLabel->SetFontSize(32.0f);
	handLabel->SetText(PString("손패 Hand (2)"));

	// 비활성 버튼: 꺼진 모습으로 그리고, 눌러도 클릭이 아니지만 월드로 넘기지도 않는다
	PSharedPtr<PGameUIButton> draw = AddChild<PGameUIButton>(PName("Draw"));
	draw->SetLayout(HVector2(1.0f, 1.0f), HVector2(1.0f, 1.0f), HVector2(-400.0f, -40.0f), HVector2(240.0f, 110.0f));
	draw->SetLabel(PString("Draw (off)"));
	draw->SetEnabled(false);
	draw->OnClicked.AddLambda([]()
		{
			JG_LOG(GameUIDemo, ELogLevel::Error, "GameUIDemo Draw clicked while disabled");
		});

	// 턴 종료 버튼: 오른쪽 아래, 주황 스타일
	PSharedPtr<PGameUIButton> endTurn = AddChild<PGameUIButton>(PName("EndTurn"));
	endTurn->SetLayout(HVector2(1.0f, 1.0f), HVector2(1.0f, 1.0f), HVector2(-40.0f, -40.0f), HVector2(340.0f, 110.0f));
	endTurn->SetStyle(makeEndTurnStyle());
	endTurn->SetLabel(PString("End Turn · 턴 종료"));
	endTurn->OnClicked.AddLambda([this]()
		{
			onEndTurnClicked();
		});

	// 자동 줄바꿈(ER-007): 오른쪽 패널. 폭 600 글자 요소에 긴 문단(단어 단위), 높이는 MeasureContent 로 구해 패널을 맞춘다.
	// 그 아래 한 줄은 줄을 바꾸지 않고 말줄임으로 줄인다.
	PSharedPtr<PGameUIImage> wrapPanel = AddChild<PGameUIImage>(PName("WrapPanel"));
	wrapPanel->SetColor(HLinearColor(0.02f, 0.03f, 0.05f, 0.72f));
	wrapPanel->SetHitTestVisible(true);

	PSharedPtr<PGameUIText> paragraph = wrapPanel->AddChild<PGameUIText>(PName("Paragraph"));
	paragraph->SetFontSize(26.0f);
	paragraph->SetWrap(EGameUITextWrap::Word);
	paragraph->SetClip(true);
	paragraph->SetText(PString("자동 줄바꿈 확인용 문단입니다. 요소 폭을 넘는 글은 어절 단위로 다음 줄로 넘어가고, 높이는 MeasureContent 로 읽어 패널 크기를 맞춥니다. "
		"A word longer than the line, like https://example.com/a/very/long/path/that/does/not/fit/in/one/line, breaks inside the width."));
	const HVector2 paragraphSize = paragraph->MeasureContent(600.0f, GetManager());
	paragraph->SetLayout(HVector2(0.0f, 0.0f), HVector2(0.0f, 0.0f), HVector2(20.0f, 16.0f), HVector2(600.0f, paragraphSize.y));

	PSharedPtr<PGameUIText> ellipsis = wrapPanel->AddChild<PGameUIText>(PName("Ellipsis"));
	ellipsis->SetFontSize(26.0f);
	ellipsis->SetColor(HLinearColor(0.72f, 0.80f, 0.92f, 1.0f));
	ellipsis->SetEllipsis(true);
	ellipsis->SetText(PString("말줄임: 이 줄은 요소 폭보다 길어서 끝이 말줄임표로 줄어듭니다 — the end of this line is cut"));
	const float32 ellipsisHeight = ellipsis->MeasureContent(600.0f, GetManager()).y;
	ellipsis->SetLayout(HVector2(0.0f, 0.0f), HVector2(0.0f, 0.0f), HVector2(20.0f, 16.0f + paragraphSize.y + 12.0f), HVector2(600.0f, ellipsisHeight));
	wrapPanel->SetLayout(HVector2(1.0f, 0.0f), HVector2(1.0f, 0.0f), HVector2(-40.0f, 130.0f), HVector2(640.0f, 16.0f + paragraphSize.y + 12.0f + ellipsisHeight + 16.0f));
	JG_LOG(GameUIDemo, ELogLevel::Info, "GameUIDemo paragraph measured %.1f x %.1f (width 600)", paragraphSize.x, paragraphSize.y);

	// 글자 입력(ER-008): 주소(숫자 · '.' · ':' 만, 21자)와 이름(16자). Enter 로 확정하면 로그에 남긴다.
	PSharedPtr<PGameUITextInput> address = AddChild<PGameUITextInput>(PName("Address"));
	address->SetLayout(HVector2(1.0f, 0.0f), HVector2(1.0f, 0.0f), HVector2(-40.0f, 600.0f), HVector2(640.0f, 60.0f));
	address->SetPlaceholder(PString("주소 127.0.0.1:47770"));
	address->SetAllowedCharacters(PString("0123456789.:"));
	address->SetMaxLength(21);
	address->OnCommitted.AddLambda([](const PString& text)
		{
			JG_LOG(GameUIDemo, ELogLevel::Info, "GameUIDemo address committed: %s", text);
		});

	PSharedPtr<PGameUITextInput> name = AddChild<PGameUITextInput>(PName("Name"));
	name->SetLayout(HVector2(1.0f, 0.0f), HVector2(1.0f, 0.0f), HVector2(-40.0f, 680.0f), HVector2(640.0f, 60.0f));
	name->SetPlaceholder(PString("이름 Name"));
	name->SetMaxLength(16);
	name->OnCommitted.AddLambda([](const PString& text)
		{
			JG_LOG(GameUIDemo, ELogLevel::Info, "GameUIDemo name committed: %s", text);
		});

	JG_LOG(GameUIDemo, ELogLevel::Info, "GameUIDemo HUD initialized: %d root elements", (int32)GetRoot()->GetChildren().size());
}

void JGGameUIDemoHUD::OnActivated()
{
	JG_LOG(GameUIDemo, ELogLevel::Info, "GameUIDemo HUD activated");
}

void JGGameUIDemoHUD::OnDeactivated()
{
	JG_LOG(GameUIDemo, ELogLevel::Info, "GameUIDemo HUD deactivated");
}

void JGGameUIDemoHUD::updateTurnText()
{
	if (_turnText.IsValid())
	{
		_turnText->SetText(PString::Format("GameUI 데모 · 턴 %d", _turn));
	}
}

void JGGameUIDemoHUD::onEndTurnClicked()
{
	++_turn;
	updateTurnText();
	JG_LOG(GameUIDemo, ELogLevel::Info, "GameUIDemo End Turn clicked (turn %d)", _turn);
}

void JGGameUIDemoHUD::onMenuClicked()
{
	JG_LOG(GameUIDemo, ELogLevel::Info, "GameUIDemo Menu clicked");
	JGGameInstance::Get().GetUI()->PushWidget<JGGameUIDemoPauseMenu>(EGameUILayer::Menu);
}
