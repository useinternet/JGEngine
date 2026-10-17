#include "PCH/PCH.h"
#include "GameUIDemoHUD.h"
#include "GameUIDemoPauseMenu.h"
#include "UI/GameUI.h"
#include <cmath>

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

	// 끌어다 놓기(GG-15): 놓을 곳 색(보통 · 끄는 카드가 위에 있을 때)과 놓은 카드 자리(놓을 곳 오른쪽, 기준 해상도 아래 왼쪽 기준).
	const HLinearColor DropZoneColor(0.02f, 0.03f, 0.05f, 0.55f);
	const HLinearColor DropZoneHotColor(0.18f, 0.48f, 0.30f, 0.80f);
	const HVector2     CardInZonePosition(920.0f, -40.0f);

	// 휠 받기(GG-21) 지도: 칸 160 × 150 을 8 × 2(1280 × 300), 처음 배율 0.5 = 틀(640 × 150)에 꼭 맞음. 휠 한 칸 = 1.15배.
	constexpr float32 MapMinScale    = 0.5f;
	constexpr float32 MapMaxScale    = 2.0f;
	constexpr float32 MapZoomStep    = 1.15f;
	constexpr float32 MapZoomSeconds = 0.12f;
	const char* const MapColumnNames[8] = { "A", "B", "C", "D", "E", "F", "G", "H" };
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

	// 놓을 곳(GG-15): 가운데 아래. 카드보다 먼저 붙여 끄는 카드가 그 위에 그려지게 한다. 입력을 받아야 놓을 곳(판정 대상)이 된다.
	_dropZone = AddChild<PGameUIImage>(PName("DropZone"));
	_dropZone->SetLayout(HVector2(0.0f, 1.0f), HVector2(0.0f, 1.0f), HVector2(700.0f, -40.0f), HVector2(420.0f, 251.0f));
	_dropZone->SetColor(DropZoneColor);
	_dropZone->SetHitTestVisible(true);
	PSharedPtr<PGameUIText> dropLabel = _dropZone->AddChild<PGameUIText>(PName("DropLabel"));
	dropLabel->SetLayout(HVector2(0.0f, 0.0f), HVector2(0.0f, 0.0f), HVector2(16.0f, 0.0f), HVector2(190.0f, 251.0f));
	dropLabel->SetFontSize(28.0f);
	dropLabel->SetColor(HLinearColor(0.72f, 0.80f, 0.92f, 1.0f));
	dropLabel->SetAlignment(EGameUIHorizontalAlign::Left, EGameUIVerticalAlign::Middle);
	dropLabel->SetText(PString("놓을 곳\nDrop zone"));

	// 스크롤 칸(GG-20) · 휠로 확대하는 지도(GG-21). 카드보다 먼저 붙여 끄는 카드가 그 위에 그려지게 한다.
	addScrollBoxes();
	addMap();

	// 손패: 엔진 Content 의 카드 뒷면 두 장. 두 번째(금색)는 끌 수 있다.
	const char* const cardFiles[2] = { "RawResources/CardBack_Tree_Dark_63x88mm_300dpi.png", "RawResources/CardBack_Tree_Gold_63x88mm_300dpi.png" };
	for (int32 i = 0; i < 2; ++i)
	{
		PString cardPath;
		HFileHelper::CombinePath(HFileHelper::EngineContentDirectory(), PString(cardFiles[i]), &cardPath);

		PSharedPtr<PGameUIImage> card = AddChild<PGameUIImage>(PName(PString::Format("Card%d", i)));
		card->SetLayout(HVector2(0.0f, 1.0f), HVector2(0.0f, 1.0f), HVector2(40.0f + (float32)i * 200.0f, -40.0f), HVector2(180.0f, 251.0f));
		card->SetTexture(ui->LoadTexture(cardPath));
		card->SetHitTestVisible(true);
		if (i == 0)
		{
			// 요소 변환(GG-14) 화면 확인: 배치는 그대로 두고 그리기만 -5° 기울인다(기준점 = 피벗, 왼쪽 아래).
			HGameUIRenderTransform tilt;
			tilt.Angle = -5.0f;
			card->SetRenderTransform(tilt);
		}
		if (i == 1)
		{
			card->SetDraggable(true);
			_dragCard = card;
		}
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
	// 피벗은 가운데(눌림 반응 배율이 버튼 가운데를 기준으로 한다). 자리는 오른쪽 · 아래 여백 40 그대로(1540~1880 × 930~1040).
	PSharedPtr<PGameUIButton> endTurn = AddChild<PGameUIButton>(PName("EndTurn"));
	endTurn->SetLayout(HVector2(1.0f, 1.0f), HVector2(0.5f, 0.5f), HVector2(-210.0f, -95.0f), HVector2(340.0f, 110.0f));
	_endTurn = endTurn;
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

	// 글 서식(ER-010) · 글꼴(ER-011): 왼쪽 패널. 줄마다 높이를 MeasureContent 로 구해 아래로 쌓는다.
	// 패널은 입력을 받지 않는다(빈 곳 클릭은 월드로 — 검증 스크립트의 월드 클릭 자리와 겹친다).
	PSharedPtr<PGameUIImage> formatPanel = AddChild<PGameUIImage>(PName("FormatPanel"));
	formatPanel->SetColor(HLinearColor(0.02f, 0.03f, 0.05f, 0.72f));
	const float32 formatWidth = 1060.0f;
	float32       formatY     = 16.0f;
	auto placeLine = [this, &formatY, formatWidth](PSharedPtr<PGameUIText> line)
		{
			const float32 lineHeight = line->MeasureContent(formatWidth, GetManager()).y;
			line->SetLayout(HVector2(0.0f, 0.0f), HVector2(0.0f, 0.0f), HVector2(20.0f, formatY), HVector2(formatWidth, lineHeight));
			formatY += lineHeight + 12.0f;
		};

	// 구간 서식: 글 안 마크업(굵게 · 기울임 · 색 · 장평). 글자 그대로의 꺾쇠는 &lt; &gt;.
	PSharedPtr<PGameUIText> rich = formatPanel->AddChild<PGameUIText>(PName("RichText"));
	rich->SetFontSize(30.0f);
	rich->SetWrap(EGameUITextWrap::Word);
	rich->SetRichText(true);
	rich->SetText(PString("<b>굵게 Bold</b> · 보통 · <i>기울임 Italic</i> · <color=#FF5A5A>빨강</color> <color=#5AC8FF>파랑</color> · "
		"<hscale=140>넓게 140%</hscale> <hscale=70>좁게 70%</hscale> · &lt;b&gt; 는 글자"));
	placeLine(rich);

	// 문단 서식: 양쪽 맞춤 · 줄 간격 · 자간 · 문단 간격
	PSharedPtr<PGameUIText> justified = formatPanel->AddChild<PGameUIText>(PName("Justified"));
	justified->SetFontSize(24.0f);
	justified->SetWrap(EGameUITextWrap::Word);
	justified->SetAlignment(EGameUIHorizontalAlign::Justify, EGameUIVerticalAlign::Top);
	HGameUIParagraphStyle justifiedStyle;
	justifiedStyle.LineHeight       = 36.0f;
	justifiedStyle.LetterSpacing    = 20.0f;
	justifiedStyle.ParagraphSpacing = 10.0f;
	justified->SetParagraphStyle(justifiedStyle);
	justified->SetText(PString("양쪽 맞춤 문단: 자동으로 바뀐 줄은 남는 폭을 글자 사이에 나눠 오른쪽 끝을 맞추고, 띄어쓰기 뒤 사이에는 네 배를 줍니다. "
		"줄 간격 36 · 자간 +20/1000 em. Justified lines end exactly at the right edge of the element.\n"
		"문단의 마지막 줄은 왼쪽 정렬 그대로입니다(문단 간격 10)."));
	placeLine(justified);

	// 요소 전체 서식: 합성 굵게 · 합성 기울임 · 장평
	PSharedPtr<PGameUIText> wholeStyled = formatPanel->AddChild<PGameUIText>(PName("WholeStyled"));
	wholeStyled->SetFontSize(26.0f);
	wholeStyled->SetColor(HLinearColor(0.72f, 0.80f, 0.92f, 1.0f));
	HGameUIParagraphStyle wholeStyle;
	wholeStyle.bBold           = true;
	wholeStyle.bItalic         = true;
	wholeStyle.HorizontalScale = 0.9f;
	wholeStyled->SetParagraphStyle(wholeStyle);
	wholeStyled->SetText(PString("요소 전체: 합성 굵게 + 기울임 + 장평 90% (synthetic bold italic)"));
	placeLine(wholeStyled);

	// 대체 글꼴: Arial 에 없는 한글을 맑은 고딕으로 배치 · 그린다.
	PSharedPtr<PGameUIFont> latinFont  = ui->LoadFont(PString("C:/Windows/Fonts/arial.ttf"));
	PSharedPtr<PGameUIFont> hangulFont = ui->LoadFont(PString("C:/Windows/Fonts/malgun.ttf"));
	PSharedPtr<PGameUIText> fallback   = formatPanel->AddChild<PGameUIText>(PName("Fallback"));
	if (latinFont.IsValid() && hangulFont.IsValid())
	{
		latinFont->SetFallbackFont(hangulFont);
		fallback->SetFont(latinFont);
	}
	fallback->SetFontSize(28.0f);
	fallback->SetText(PString("Arial + fallback font: Café naïve · 한글은 맑은 고딕으로"));
	placeLine(fallback);

	// 아틀라스 페이지: 관리자 캐시와 따로 연 글꼴. 위 줄은 첫 프레임에 페이지 0, 아래 줄은 페이지 0 을 채운 뒤 페이지 1 에 들어간다.
	_atlasFont = Allocate<PGameUIFont>();
	_atlasFont->LoadFromFile(PString("C:/Windows/Fonts/malgun.ttf"));
	PSharedPtr<PGameUIText> atlasEarly = formatPanel->AddChild<PGameUIText>(PName("AtlasEarly"));
	atlasEarly->SetFont(_atlasFont);
	atlasEarly->SetFontSize(30.0f);
	atlasEarly->SetText(PString("페이지 0 글자: 가나다라마바사 ABC"));
	placeLine(atlasEarly);

	_atlasLate = formatPanel->AddChild<PGameUIText>(PName("AtlasLate"));
	_atlasLate->SetFont(_atlasFont);
	_atlasLate->SetFontSize(30.0f);
	_atlasLate->SetText(PString("페이지 1 글자: 아자차카타파하 XYZ"));
	placeLine(_atlasLate);
	_atlasLate->SetText(PString());

	_atlasStats = formatPanel->AddChild<PGameUIText>(PName("AtlasStats"));
	_atlasStats->SetFontSize(22.0f);
	_atlasStats->SetColor(HLinearColor(0.72f, 0.80f, 0.92f, 1.0f));
	_atlasStats->SetText(PString("atlas pages -"));
	placeLine(_atlasStats);

	formatPanel->SetLayout(HVector2(0.0f, 0.0f), HVector2(0.0f, 0.0f), HVector2(40.0f, 130.0f), HVector2(formatWidth + 40.0f, formatY + 4.0f));

	// 굵게 세기(GG-18, ER-018) · 선명한 굵게(GG-19, ER-019): 같은 <b> 구간을 요소 세기만 바꿔 세 줄 — 기본(세기 0 = 지금 넓힘, 옅은 테) ·
	// em/28(선명한 굵게 — 기본 넓힘의 em/16 과 같은 굵기) · em/20(선명한 굵게, 더 굵게). 오른쪽 입력란과 End Turn 사이(입력을 받지 않는다).
	// 40 논리 단위 = 에디터 씬 뷰포트(800 픽셀 폭)에서 약 17px — 게임 화면 글(14~19px) 크기.
	PSharedPtr<PGameUIImage> boldPanel = AddChild<PGameUIImage>(PName("BoldStrengthPanel"));
	boldPanel->SetLayout(HVector2(1.0f, 0.0f), HVector2(1.0f, 0.0f), HVector2(-40.0f, 760.0f), HVector2(640.0f, 156.0f));
	boldPanel->SetColor(HLinearColor(0.02f, 0.03f, 0.05f, 0.72f));
	const char* const   boldLabels[3]     = { "기본", "em/28", "em/20" };
	const float32       boldStrengths[3]  = { 0.0f, 1.0f / 28.0f, 1.0f / 20.0f };
	for (int32 i = 0; i < 3; ++i)
	{
		PSharedPtr<PGameUIText> line = boldPanel->AddChild<PGameUIText>(PName(PString::Format("Bold%d", i)));
		line->SetLayout(HVector2(0.0f, 0.0f), HVector2(0.0f, 0.0f), HVector2(16.0f, 6.0f + (float32)i * 50.0f), HVector2(608.0f, 48.0f));
		line->SetFontSize(40.0f);
		line->SetRichText(true);
		HGameUIParagraphStyle boldStyle;
		boldStyle.BoldStrength = boldStrengths[i];
		line->SetParagraphStyle(boldStyle);
		line->SetText(PString::Format("%s <b>폭로 – 강제 – 전투.</b> 본문", PString(boldLabels[i])));
	}

	JG_LOG(GameUIDemo, ELogLevel::Info, "GameUIDemo HUD initialized: %d root elements", (int32)GetRoot()->GetChildren().size());
}

void JGGameUIDemoHUD::addScrollBoxes()
{
	// 목록 칸: 카드(40~420)와 놓을 곳(700~) 사이 450~680 × 789~1040. 줄(320 폭)이 보이는 칸(214)보다 넓어 세로 · 가로 막대가 함께 보인다.
	PSharedPtr<PGameUIImage> listPanel = AddChild<PGameUIImage>(PName("ListPanel"));
	listPanel->SetLayout(HVector2(0.0f, 1.0f), HVector2(0.0f, 1.0f), HVector2(450.0f, -40.0f), HVector2(230.0f, 251.0f));
	listPanel->SetColor(HLinearColor(0.02f, 0.03f, 0.05f, 0.72f));
	PSharedPtr<PGameUIScrollBox> list = listPanel->AddChild<PGameUIScrollBox>(PName("ListScroll"));
	list->SetLayout(HVector2(0.0f, 0.0f), HVector2(0.0f, 0.0f), HVector2(0.0f, 0.0f), HVector2(230.0f, 251.0f));
	list->SetScrollDirection(EGameUIScrollDirection::Both);
	list->SetScrollKey(PName("DemoList"));

	HGameUIButtonStyle rowStyle;
	rowStyle.Normal         = HLinearColor(0.12f, 0.15f, 0.20f, 1.0f);
	rowStyle.Hovered        = HLinearColor(0.22f, 0.28f, 0.36f, 1.0f);
	rowStyle.Pressed        = HLinearColor(0.08f, 0.10f, 0.13f, 1.0f);
	rowStyle.Label.FontSize = 20.0f;
	for (int32 i = 0; i < 40; ++i)
	{
		PSharedPtr<PGameUIButton> row = list->GetContent()->AddChild<PGameUIButton>(PName(PString::Format("ListRow%d", i)));
		row->SetLayout(HVector2(0.0f, 0.0f), HVector2(0.0f, 0.0f), HVector2(0.0f, (float32)i * 40.0f), HVector2(320.0f, 36.0f));
		row->SetStyle(rowStyle);
		row->SetLabel(PString::Format("목록 %02d · list row %02d", i, i));
		row->OnClicked.AddLambda([i]()
			{
				JG_LOG(GameUIDemo, ELogLevel::Info, "GameUIDemo list row %d clicked", i);
			});
	}
	list->OnScrolled.AddLambda([](const HVector2& offset)
		{
			JG_LOG(GameUIDemo, ELogLevel::Info, "GameUIDemo scroll ListScroll offset (%.1f, %.1f)", offset.x, offset.y);
		});

	// 이야기 칸: 가운데 680~1220 × 640~780(6줄 남짓). 20줄 글, 세로 막대, 휠 · 막대 빈 곳 누르기를 0.15초로 부드럽게.
	PSharedPtr<PGameUIImage> storyPanel = AddChild<PGameUIImage>(PName("StoryPanel"));
	storyPanel->SetLayout(HVector2(0.0f, 0.0f), HVector2(0.0f, 0.0f), HVector2(680.0f, 640.0f), HVector2(540.0f, 140.0f));
	storyPanel->SetColor(HLinearColor(0.02f, 0.03f, 0.05f, 0.72f));
	PSharedPtr<PGameUIScrollBox> story = storyPanel->AddChild<PGameUIScrollBox>(PName("StoryScroll"));
	story->SetLayout(HVector2(0.0f, 0.0f), HVector2(0.0f, 0.0f), HVector2(0.0f, 0.0f), HVector2(540.0f, 140.0f));
	HGameUIScrollStyle storyStyle;
	storyStyle.SmoothSeconds = 0.15f;
	story->SetStyle(storyStyle);

	PString storyText;
	for (int32 line = 1; line <= 20; ++line)
	{
		const PString lineText = PString::Format("%02d줄 · 긴 글은 칸 안에서 넘겨 봅니다 (story line %d)", line, line);
		storyText = storyText.Empty() ? lineText : PString::Format("%s\n%s", storyText, lineText);
	}
	PSharedPtr<PGameUIText> storyLines = story->GetContent()->AddChild<PGameUIText>(PName("StoryText"));
	storyLines->SetFontSize(18.0f);
	HGameUIParagraphStyle storyParagraph;
	storyParagraph.LineHeight = 22.0f;
	storyLines->SetParagraphStyle(storyParagraph);
	storyLines->SetText(storyText);
	// 글 폭 = 칸 폭 − 막대 자리(12 + 2 × 2) − 왼쪽 여백 8. 높이는 글에 맞춘다(내용 크기 자동).
	const float32 storyWidth = 540.0f - 16.0f - 8.0f;
	storyLines->SetLayout(HVector2(0.0f, 0.0f), HVector2(0.0f, 0.0f), HVector2(8.0f, 0.0f), HVector2(storyWidth, storyLines->MeasureContent(storyWidth, GetManager()).y));
	story->OnScrolled.AddLambda([](const HVector2& offset)
		{
			JG_LOG(GameUIDemo, ELogLevel::Info, "GameUIDemo scroll StoryScroll offset (%.1f, %.1f)", offset.x, offset.y);
		});
	JG_LOG(GameUIDemo, ELogLevel::Info, "GameUIDemo scroll boxes: list max (%.1f, %.1f), story max (%.1f, %.1f)",
		list->GetMaxScroll().x, list->GetMaxScroll().y, story->GetMaxScroll().x, story->GetMaxScroll().y);
}

void JGGameUIDemoHUD::addMap()
{
	// 틀: 오른쪽 줄바꿈 패널과 입력란 사이 1240~1880 × 430~580. 자식 잘라내기 · 입력 받기 · 끌기 · 휠 받기를 켠다 — 지도가 작아져도 틀이 받는다.
	_mapFrame = AddChild<PGameUIImage>(PName("MapFrame"));
	_mapFrame->SetLayout(HVector2(1.0f, 0.0f), HVector2(1.0f, 0.0f), HVector2(-40.0f, 430.0f), HVector2(640.0f, 150.0f));
	_mapFrame->SetColor(HLinearColor(0.02f, 0.03f, 0.05f, 0.72f));
	_mapFrame->SetClipChildren(true);
	_mapFrame->SetHitTestVisible(true);
	_mapFrame->SetDraggable(true);
	_mapFrame->SetWheelInput(true);
	_mapFrame->OnWheel.AddLambda([this](PGameUIElement& element, float32 wheel, float32 wheelHorizontal, const HVector2& pointerPoint)
		{
			zoomMap(pointerPoint, wheel);
		});

	// 지도: 피벗 왼쪽 위, 그리기 변환으로만 확대 · 옮기기(배치는 그대로). 칸 · 글은 입력을 받지 않는다(틀이 받는다).
	_mapContent = _mapFrame->AddChild<PGameUIElement>(PName("MapContent"));
	_mapContent->SetLayout(HVector2(0.0f, 0.0f), HVector2(0.0f, 0.0f), HVector2(0.0f, 0.0f), HVector2(1280.0f, 300.0f));
	HGameUIRenderTransform start;
	start.Scale = HVector2(MapMinScale, MapMinScale);
	_mapContent->SetRenderTransform(start);
	_mapTargetScale = MapMinScale;
	for (int32 row = 0; row < 2; ++row)
	{
		for (int32 column = 0; column < 8; ++column)
		{
			const bool bDark = ((row + column) % 2) == 0;
			PSharedPtr<PGameUIImage> tile = _mapContent->AddChild<PGameUIImage>(PName(PString::Format("MapTile%d_%d", column, row)));
			tile->SetLayout(HVector2(0.0f, 0.0f), HVector2(0.0f, 0.0f), HVector2((float32)column * 160.0f, (float32)row * 150.0f), HVector2(160.0f, 150.0f));
			tile->SetColor(bDark ? HLinearColor(0.14f, 0.22f, 0.17f, 1.0f) : HLinearColor(0.20f, 0.30f, 0.23f, 1.0f));
			PSharedPtr<PGameUIText> label = tile->AddChild<PGameUIText>(PName("Label"));
			label->SetLayout(HVector2(0.0f, 0.0f), HVector2(0.0f, 0.0f), HVector2(0.0f, 0.0f), HVector2(160.0f, 150.0f));
			label->SetFontSize(36.0f);
			label->SetAlignment(EGameUIHorizontalAlign::Center, EGameUIVerticalAlign::Middle);
			label->SetText(PString::Format("장소 %s%d", PString(MapColumnNames[column]), row + 1));
		}
	}
}

HVector2 JGGameUIDemoHUD::mapPivotPoint() const
{
	const HRect&    rect  = _mapContent->GetLayoutRect();
	const HVector2& pivot = _mapContent->GetPivot();
	return HVector2(rect.left + pivot.x * (rect.right - rect.left), rect.top + pivot.y * (rect.bottom - rect.top));
}

void JGGameUIDemoHUD::zoomMap(const HVector2& pointerPoint, float32 wheel)
{
	// 포인터 기준 확대(GG-21): q = 지도.CanvasToLocal(포인터), P = 지도 피벗 자리, 목표 배율 s' 에서 옮기기 t' = t − (s' − s) × (q − P).
	// 배율 · 옮기기 트윈을 한 동시 묶음(같은 길이 · 이징)으로 걸면 움직이는 동안에도 q 가 포인터 아래 그대로다. 연달아 굴리면 목표 배율을 쌓고
	// 새 묶음이 지금 값에서 이어 간다(같은 속성이라 옛 묶음은 취소). 칸 수가 소수(정밀 휠)여도 1.15 ^ 칸 수.
	const HGameUIRenderTransform current = _mapContent->GetRenderTransform();
	const float32  targetScale = HMath::Clamp(_mapTargetScale * std::pow(MapZoomStep, wheel), MapMinScale, MapMaxScale);
	const HVector2 local       = _mapContent->CanvasToLocal(pointerPoint);
	const HVector2 pivot       = mapPivotPoint();
	const HVector2 targetTranslation(current.Translation.x - (targetScale - current.Scale.x) * (local.x - pivot.x),
		current.Translation.y - (targetScale - current.Scale.y) * (local.y - pivot.y));
	_mapTargetScale = targetScale;
	_mapZoomLocal   = local;
	_mapZoomPointer = pointerPoint;

	const bool bDragging = GetManager()->GetDraggedElement().GetRawPointer() == _mapFrame.GetRawPointer();
	JG_LOG(GameUIDemo, ELogLevel::Info, "GameUIDemo map zoom wheel %.2f -> scale %.3f at (%.0f, %.0f)%s", wheel, targetScale, pointerPoint.x, pointerPoint.y,
		PString(bDragging ? " (dragging)" : ""));
	if (bDragging)
	{
		// 끄는 중: 끌기가 이동마다 옮기기를 정하므로 바로 바꾸고, 끌기 기준도 같은 만큼 옮긴다.
		HGameUIRenderTransform next = current;
		next.Scale       = HVector2(targetScale, targetScale);
		next.Translation = targetTranslation;
		_mapContent->SetRenderTransform(next);
		_mapDragTranslation = _mapDragTranslation + (targetTranslation - current.Translation);
		logMapAnchor();
		return;
	}

	PSharedPtr<PGameUITimeline> zoom = PGameUITimeline::Parallel();
	zoom->Add(HGameUITween::To(_mapContent, EGameUITweenProperty::Scale, HVector2(targetScale, targetScale), MapZoomSeconds, EGameUIEase::OutCubic))
		.Add(HGameUITween::To(_mapContent, EGameUITweenProperty::Translation, targetTranslation, MapZoomSeconds, EGameUIEase::OutCubic));
	zoom->OnFinished.AddLambda([this](bool bCanceled)
		{
			// 다음 휠이 이어 받아 취소된 묶음은 건너뛴다(끝난 묶음만 확인).
			if (bCanceled == false)
			{
				logMapAnchor();
			}
		});
	_mapZoom = PlayTimeline(zoom);
}

void JGGameUIDemoHUD::logMapAnchor()
{
	// 그 휠 때 포인터 아래였던 지도 점의 지금 화면 자리(틀 · 지도 위로 변환이 없어 캔버스 = P + t + s × (q − P)).
	const HGameUIRenderTransform transform = _mapContent->GetRenderTransform();
	const HVector2 pivot = mapPivotPoint();
	const HVector2 anchor(pivot.x + transform.Translation.x + transform.Scale.x * (_mapZoomLocal.x - pivot.x),
		pivot.y + transform.Translation.y + transform.Scale.y * (_mapZoomLocal.y - pivot.y));
	JG_LOG(GameUIDemo, ELogLevel::Info, "GameUIDemo map zoom done scale %.3f, point under the pointer moved %.2f", transform.Scale.x, HVector2::Length(anchor - _mapZoomPointer));
}

void JGGameUIDemoHUD::OnUpdate(float32 deltaSeconds)
{
	// 위 줄 글자가 한 번 그려진 뒤(아틀라스에 글리프가 생긴 뒤) 한 번만 채운다.
	if (_bAtlasFilled == false && _atlasFont.IsValid() && _atlasFont->GetGlyphCount() > 0)
	{
		fillAtlasPage();
	}
}

void JGGameUIDemoHUD::fillAtlasPage()
{
	_bAtlasFilled = true;

	// 그리지 않는 큰 글자(128px)로 페이지 0 을 채워 새 페이지를 열게 한다. 이 글자들은 아래 줄과 겹치지 않는다(U+B000 부터).
	uint32 codepoint = 0xB000;
	while (_atlasFont->GetAtlasPageCount() < 2 && codepoint < 0xB400)
	{
		_atlasFont->GetGlyph(codepoint++, 128);
	}
	_atlasLate->SetText(PString("페이지 1 글자: 아자차카타파하 XYZ"));

	const HGameUIFontAtlasStats stats = _atlasFont->GetAtlasStats();
	_atlasStats->SetText(PString::Format("ER-011 atlas: pages %d / limit %d · usage %d%% · evicted %d (filler %d glyphs at 128px)",
		stats.PageCount, stats.PageLimit, (int32)(stats.Usage * 100.0f + 0.5f), (int32)stats.EvictedPageCount, (int32)(codepoint - 0xB000)));
	JG_LOG(GameUIDemo, ELogLevel::Info, "GameUIDemo atlas filled: pages %d, filler %d glyphs", stats.PageCount, (int32)(codepoint - 0xB000));
}

void JGGameUIDemoHUD::OnActivated()
{
	// 끌기 이벤트는 활성인 동안만 받는다(스택이 바뀌어 끌기가 취소되면 그 이벤트가 비활성 훅보다 먼저 온다).
	PGameUIManager* ui = GetManager();
	ui->OnDragBegin.AddRaw(this, &JGGameUIDemoHUD::onDragBegin);
	ui->OnDragMove.AddRaw(this, &JGGameUIDemoHUD::onDragMove);
	ui->OnDragEnd.AddRaw(this, &JGGameUIDemoHUD::onDragEnd);
	JG_LOG(GameUIDemo, ELogLevel::Info, "GameUIDemo HUD activated");

	// 움직임(GG-16): 처음 활성 때 카드 두 장이 왼쪽 밖에서 날아온다(옮기기 · 불투명도, 동시 묶음). 첫 카드의 -5° 기울기는 그대로 둔다.
	if (_bCardsFlown == false)
	{
		_bCardsFlown = true;
		PSharedPtr<PGameUITimeline> flyIn = PGameUITimeline::Parallel();
		for (int32 i = 0; i < 2; ++i)
		{
			PSharedPtr<PGameUIElement> card = FindElement(PName(PString::Format("Card%d", i)));
			const float32 duration = 0.5f + 0.2f * (float32)i;
			flyIn->Add(HGameUITween::FromTo(card, EGameUITweenProperty::Translation, HVector2(-700.0f, 0.0f), HVector2(0.0f, 0.0f), duration, EGameUIEase::OutCubic))
				.Add(HGameUITween::FromTo(card, EGameUITweenProperty::Opacity, 0.0f, 1.0f, duration * 0.6f, EGameUIEase::Linear));
		}
		flyIn->OnFinished.AddLambda([](bool bCanceled)
			{
				JG_LOG(GameUIDemo, ELogLevel::Info, "GameUIDemo tween CardFlyIn finished (canceled %d)", bCanceled ? 1 : 0);
			});
		PlayTimeline(flyIn);
	}
}

void JGGameUIDemoHUD::OnDeactivated()
{
	PGameUIManager* ui = GetManager();
	ui->OnDragBegin.RemoveObject(this);
	ui->OnDragMove.RemoveObject(this);
	ui->OnDragEnd.RemoveObject(this);
	JG_LOG(GameUIDemo, ELogLevel::Info, "GameUIDemo HUD deactivated");
}

void JGGameUIDemoHUD::onDragBegin(const PSharedPtr<PGameUIElement>& element, const HVector2& position)
{
	if (element.GetRawPointer() == _mapFrame.GetRawPointer())
	{
		// 지도 옮기기: 도는 확대는 끝값으로 건너뛰고(끌기와 같은 옮기기를 두고 다투지 않게) 지금 옮기기에서 시작한다.
		if (_mapZoom.IsValid() && _mapZoom->IsPlaying())
		{
			_mapZoom->Skip();
		}
		_mapDragGrab        = position;
		_mapDragTranslation = _mapContent->GetRenderTransform().Translation;
		_mapDragMoves       = 0;
		JG_LOG(GameUIDemo, ELogLevel::Info, "GameUIDemo map pan begin at (%.0f, %.0f)", position.x, position.y);
		return;
	}
	if (element.GetRawPointer() != _dragCard.GetRawPointer())
	{
		return;
	}
	_dragGrab  = position;
	_dragMoves = 0;
	_dragCard->SetOpacity(0.85f);
	JG_LOG(GameUIDemo, ELogLevel::Info, "GameUIDemo drag begin %s at (%.0f, %.0f)", element->GetName().ToString(), position.x, position.y);
}

void JGGameUIDemoHUD::onDragMove(const PSharedPtr<PGameUIElement>& element, const HVector2& position, const PSharedPtr<PGameUIElement>& target)
{
	if (element.GetRawPointer() == _mapFrame.GetRawPointer())
	{
		// 시작 옮기기 + 누른 자리에서 온 거리(이동마다 처음부터 — 사이 프레임의 쓰레기 자리가 남지 않는다).
		HGameUIRenderTransform transform = _mapContent->GetRenderTransform();
		transform.Translation = _mapDragTranslation + (position - _mapDragGrab);
		_mapContent->SetRenderTransform(transform);
		++_mapDragMoves;
		return;
	}
	if (element.GetRawPointer() != _dragCard.GetRawPointer())
	{
		return;
	}
	++_dragMoves;

	// 엔진은 끄는 요소를 옮기지 않는다: 배치는 그대로 두고 그리기 변환(GG-14)으로 포인터를 따라가며 들어 올린 모습으로 기울인다.
	HGameUIRenderTransform lifted;
	lifted.Angle       = 4.0f;
	lifted.Scale       = HVector2(1.05f, 1.05f);
	lifted.Translation = position - _dragGrab;
	_dragCard->SetRenderTransform(lifted);

	// 놓을 곳 강조: 이동 이벤트의 아래 대상(끄는 카드는 판정에서 빠진다).
	const bool bOverZone = target.GetRawPointer() == _dropZone.GetRawPointer();
	_dropZone->SetColor(bOverZone ? DropZoneHotColor : DropZoneColor);
}

void JGGameUIDemoHUD::onDragEnd(const PSharedPtr<PGameUIElement>& element, const HVector2& position, const PSharedPtr<PGameUIElement>& dropTarget, bool bCanceled)
{
	if (element.GetRawPointer() == _mapFrame.GetRawPointer())
	{
		const HVector2& translation = _mapContent->GetRenderTransform().Translation;
		JG_LOG(GameUIDemo, ELogLevel::Info, "GameUIDemo map pan end at (%.0f, %.0f): translation (%.0f, %.0f), scale %.3f (moves %d)%s", position.x, position.y,
			translation.x, translation.y, _mapContent->GetRenderTransform().Scale.x, _mapDragMoves, PString(bCanceled ? " - canceled" : ""));
		return;
	}
	if (element.GetRawPointer() != _dragCard.GetRawPointer())
	{
		return;
	}

	// 놓을 곳에 놓으면 그 안에 둔다. 아니면(다른 곳 · 취소) 배치를 바꾸지 않으므로 끌기 전 자리로 돌아간다.
	_dragCard->ClearRenderTransform();
	_dragCard->SetOpacity(1.0f);
	_dropZone->SetColor(DropZoneColor);
	if (bCanceled)
	{
		JG_LOG(GameUIDemo, ELogLevel::Info, "GameUIDemo drag end %s: canceled (moves %d)", element->GetName().ToString(), _dragMoves);
		return;
	}
	if (dropTarget.GetRawPointer() == _dropZone.GetRawPointer())
	{
		_dragCard->SetPosition(CardInZonePosition);
		JG_LOG(GameUIDemo, ELogLevel::Info, "GameUIDemo drag end %s: dropped on %s at (%.0f, %.0f) (moves %d)", element->GetName().ToString(), dropTarget->GetName().ToString(), position.x, position.y, _dragMoves);
		return;
	}
	const PString targetName = dropTarget.IsValid() ? dropTarget->GetName().ToString() : PString("none");
	JG_LOG(GameUIDemo, ELogLevel::Info, "GameUIDemo drag end %s: returned, drop target %s at (%.0f, %.0f) (moves %d)", element->GetName().ToString(), targetName, position.x, position.y, _dragMoves);
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

	// 눌림 반응(GG-16): 0.9배로 눌렸다가 튕기며(OutBack) 돌아온다. 연달아 누르면 새 묶음이 배율을 가져가 옛 묶음은 취소로 끝난다.
	PSharedPtr<PGameUITimeline> pop = PGameUITimeline::Sequence();
	pop->Add(HGameUITween::To(_endTurn, EGameUITweenProperty::Scale, HVector2(0.9f, 0.9f), 0.06f, EGameUIEase::OutQuad))
		.Add(HGameUITween::To(_endTurn, EGameUITweenProperty::Scale, HVector2(1.0f, 1.0f), 0.3f, EGameUIEase::OutBack));
	pop->OnFinished.AddLambda([](bool bCanceled)
		{
			JG_LOG(GameUIDemo, ELogLevel::Info, "GameUIDemo tween EndTurnPop finished (canceled %d)", bCanceled ? 1 : 0);
		});
	PlayTimeline(pop);
}

void JGGameUIDemoHUD::onMenuClicked()
{
	JG_LOG(GameUIDemo, ELogLevel::Info, "GameUIDemo Menu clicked");
	// 한 객체를 다시 올린다: 닫히는 중이면 지금 모습에서 다시 열린다(GG-16 — OnInitialize · OnShutdown 없이).
	if (_pauseMenu.IsValid() == false)
	{
		_pauseMenu = Allocate<JGGameUIDemoPauseMenu>();
	}
	JGGameInstance::Get().GetUI()->PushWidgetInstance(_pauseMenu, EGameUILayer::Menu);
}
