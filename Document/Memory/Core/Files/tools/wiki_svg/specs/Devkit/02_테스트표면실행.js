// Devkit — JGDevScene 그래픽스 테스트 표면: 초기화 → 메시 비동기 로드 → 매 프레임 렌더 · 표시 → 30프레임 뒤 리드백 검사
// 근거: Source/Runtime/Devkit/DevScene.cpp(OnInitialize · RequestMeshLoad · OnMeshLoaded · OnUpdate · OnGenerateGUI ·
//       DumpReadbackOnce · OnAlbedoReadback), Source/Runtime/GUI/Widget.cpp · GUIModule.cpp(Initialize · UpdateWidgets)
module.exports = {
  file: 'Devkit_02_테스트표면실행',
  alt: 'JGDevScene 그래픽스 테스트 표면 — 초기화 시 장면 · 렌더러(1920×1080) · 카메라를 생성하고 Sample.jgasset을 비동기 로드하여 메시 배치 후 카메라를 맞추며, 매 프레임 Render와 HGUI::Image로 표시하고, 메시 로드 30프레임 뒤 Albedo G버퍼(비동기) · 출력 텍스처(동기)를 리드백하여 PNG 2장을 저장한다',
  type: 'seq',
  colW: 200,
  participants: [
    { id: 'GUI', label: 'HGUIModule', sub: 'GUI 모듈', color: 'purple' },
    { id: 'DS', label: 'JGDevScene', sub: '테스트 표면', color: 'orange' },
    { id: 'SR', label: 'PSceneRenderer', sub: '장면 렌더러', color: 'green' },
    { id: 'AD', label: 'GAssetDatabase', sub: '에셋 로드', color: 'yellow' },
    { id: 'GA', label: 'PJGGraphicsAPI', sub: '그래픽스 API', color: 'blue' }
  ],
  steps: [
    { note: 'JGDevFeature 생성 시 한 번 — 하위 영역 초기화', over: ['GUI', 'DS'] },
    { gap: 10 },
    { from: 'GUI', to: 'DS', text: 'Initialize' },
    { from: 'DS', to: 'SR', text: 'Initialize — 1920×1080' },
    { from: 'DS', to: 'DS', text: 'PScene · 기본 카메라 생성' },
    { from: 'DS', to: 'AD', text: 'LoadAssetAsync — Sample.jgasset' },
    { from: 'AD', to: 'DS', text: '로드 완료 콜백 — 메인 스레드', dash: true },
    { from: 'DS', to: 'DS', text: '메시 배치 · FitCameraToMesh' },
    { gap: 8 },
    { note: '매 프레임 — 창이 열려 있는 동안', over: ['GUI', 'SR'] },
    { gap: 10 },
    { from: 'GUI', to: 'DS', text: 'OnUpdate — Update 단계' },
    { from: 'DS', to: 'SR', text: 'Render(Scene, Camera)' },
    { from: 'GUI', to: 'DS', text: 'OnGenerateGUI' },
    { from: 'DS', to: 'GUI', text: 'HGUI::Image — 디스플레이 텍스처' },
    { gap: 8 },
    { note: '메시 로드 30프레임 뒤 한 번 — 리드백 검사', over: ['DS', 'GA'] },
    { gap: 10 },
    { from: 'DS', to: 'GA', text: 'RequestTextureReadback — Albedo G버퍼 · 비동기' },
    { from: 'DS', to: 'GA', text: 'ReadbackTextureImmediate — 출력 텍스처 · 동기' },
    { from: 'GA', to: 'DS', text: '픽셀 반환 — 지난 프레임 최종 이미지', dash: true },
    { from: 'DS', to: 'DS', text: 'PNG 저장 — Scene_Immediate' },
    { from: 'GA', to: 'DS', text: '다음 BeginFrame에 Albedo 픽셀 콜백', dash: true },
    { from: 'DS', to: 'DS', text: '덮인 픽셀 계산 · PNG 저장 — Albedo_Async' }
  ]
};
