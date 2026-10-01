#pragma once
// 게임 코드가 include 하는 게임 UI 전체 헤더.
//
// 사용 예 (게임 모듈):
//   JGCLASS()
//   class JGMyHUD : public JGGameWidget
//   {
//       JG_GENERATED_CLASS_BODY
//   protected:
//       virtual void OnInitialize() override
//       {
//           PSharedPtr<PGameUIButton> endTurn = AddChild<PGameUIButton>(PName("EndTurn"));
//           endTurn->SetLayout(HVector2(1.0f, 1.0f), HVector2(1.0f, 1.0f), HVector2(-40.0f, -40.0f), HVector2(240.0f, 80.0f));
//           endTurn->SetLabel(PString("End Turn"));
//           endTurn->OnClicked.AddLambda([]() { /* 명령 제출 */ });
//       }
//   };
//   JGGameInstance::Get().GetUI()->PushWidget<JGMyHUD>(EGameUILayer::Game);
//
// 호스트(에디터 Scene Viewport)가 월드 위에 그리고 포인터 · 뒤로가기를 먼저 넘긴다. 게임 코드는 그리기를 부르지 않는다.
#include "UI/GameUIDefines.h"
#include "UI/GameUIStyle.h"
#include "UI/GameUIManager.h"
#include "UI/GameWidget.h"
#include "UI/GameUIElement.h"
#include "UI/GameUIImage.h"
#include "UI/GameUIText.h"
#include "UI/GameUIButton.h"
#include "UI/GameUITextInput.h"
#include "UI/GameUIFont.h"
#include "Core/GameInstance.h"
