// 0-1. PGameUIManager와 JGGameWidget — 위 = 관리자 · 위젯 · 요소 기반, 가운데 = 요소 종류 · 글꼴 · 그리기 목록, 아래 = 렌더러와 Graphics 2D 그리기
// 코드: UI/GameUIManager.h(레이어 · 스택 · 입력 라우팅 · BuildDrawList · Render), UI/GameWidget.h(훅 · 입력 모드 · 움직임), UI/GameUIElement.h,
//       UI/GameUIImage.h · GameUIText.h · GameUIButton.h · GameUITextInput.h · GameUIScrollBox.h, UI/GameUIFont.h, UI/GameUIDrawList.h, UI/GameUIRenderer.h
module.exports = {
  file: 'GameFrameWork_GameUI_00_게임UI구조',
  alt: 'PGameUIManager와 JGGameWidget — PGameUIManager는 Game · GameMenu · Menu · Modal 레이어마다 JGGameWidget 스택을 두고 각 스택 맨 위 위젯만 보이고 활성이며, 위젯은 PGameUIElement 트리(이미지 · 글자 · 버튼 · 입력란 · 스크롤 칸)를 소유하고, 글자는 PGameUIFont로 배치하며, 관리자가 만든 HGameUIDrawList를 PGameUIRenderer가 Graphics 2D 그리기로 렌더 타깃에 그린다',
  type: 'class',
  w: 1100,
  h: 680,
  classes: [
    { id: 'MGR', x: 40, y: 30, w: 320, name: 'PGameUIManager', stereo: '게임 UI 관리자 · 게임 인스턴스가 소유', color: 'purple', members: ['Game < GameMenu < Menu < Modal  // 레이어', '+PushWidget<T>(layer) · RemoveWidget()', '+HandlePointer() · HandleBackAction()', '+Render(target)'] },
    { id: 'WG', x: 450, y: 30, w: 280, name: 'JGGameWidget', stereo: '화면 하나 · 게임이 파생', color: 'purple', members: ['#OnInitialize() · OnActivated()', '#OnUpdate(dt) · OnBackAction()', 'InputMode : Game · Menu', '+PlayTimeline() · PlayTween()'] },
    { id: 'EL', x: 820, y: 30, w: 240, name: 'PGameUIElement', stereo: '요소 트리 · 논리 단위', color: 'blue', members: ['Anchor · Pivot · Position · Size', '+AddChild()', '+SetRenderTransform()'] },
    { id: 'KIND', x: 820, y: 300, w: 240, name: '요소 종류', stereo: 'PGameUIElement 상속', color: 'blue', stack: 1, members: ['PGameUIImage  // 이미지', 'PGameUIText  // 글자 · 서식', 'PGameUIButton  // 버튼', 'PGameUITextInput  // 입력란', 'PGameUIScrollBox  // 스크롤 칸'] },
    { id: 'FONT', x: 450, y: 300, w: 280, name: 'PGameUIFont', stereo: '글꼴 · 아틀라스 페이지', color: 'gray', members: ['+LayoutText(text, options)', '대체 글꼴 · 합성 굵게 · 기울임'] },
    { id: 'DL', x: 40, y: 300, w: 320, name: 'HGameUIDrawList', stereo: '그리기 목록 · 렌더 타깃 픽셀', color: 'gray', members: ['HGameUIDrawCommand  // 텍스처 · 클립별 묶음'] },
    { id: 'RND', x: 40, y: 500, w: 320, name: 'PGameUIRenderer', stereo: '렌더러 · 알파 블렌드', color: 'gray', members: ['+Render(drawList, target)'] }
  ],
  nodes: [
    { id: 'GFX', x: 450, y: 506, w: 280, h: 70, color: 'gray', title: 'IJGGraphicsCommand', lines: ['Graphics 2D 그리기', 'Draw(H2DDrawArguments)'] },
    { id: 'KEY', x: 820, y: 560, w: 240, h: 96, color: 'white', shape: 'note', title: '범례', lines: ['◇— 소유 · —▷ 상속 · → 사용', '겹친 카드 = 여러 개', '+ public · # protected'] }
  ],
  edges: [
    { points: [[360, 60], [450, 60]], start: 'diamond', end: 'none', label: '레이어별 스택' },
    { points: [[730, 60], [820, 60]], start: 'diamond', end: 'none' },
    { from: 'KIND', to: 'EL', fs: 't', ts: 'b', end: 'triangle' },
    { points: [[820, 340], [730, 340]] },
    { from: 'MGR', to: 'DL', fs: 'b', ts: 't', label: 'BuildDrawList' },
    { from: 'DL', to: 'RND', fs: 'b', ts: 't' },
    { points: [[360, 540], [450, 540]], label: '그리기' }
  ]
};
