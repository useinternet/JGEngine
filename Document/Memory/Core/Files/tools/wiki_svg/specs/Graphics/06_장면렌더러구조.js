// 5-1. PScene과 PSceneRenderer — 장면은 데이터(번호로 메시 배치 · 카메라), 렌더러는 장면을 읽어 그린다. 위 줄 = 소유하거나 번호를 든 쪽
// 코드: Classes/Scene.h/.cpp, Classes/SceneRenderer.h/.cpp, GameFrameWorks/Core/World.h, Components/StaticMeshComponent.h · CameraComponent.h, Viewport/GameViewport.h, Devkit/DevScene.h
module.exports = {
  file: 'Graphics_06_장면렌더러구조',
  alt: 'PScene과 PSceneRenderer — PWorld가 PScene을 소유하고 메시 · 카메라 컴포넌트는 번호로 배치를 생성 · 수정하며, PScene은 HSceneMesh와 HSceneCamera를 번호별로 보관하고, PGameViewport · JGDevScene이 소유한 PSceneRenderer가 Render 시 장면을 읽어 그리기 명령으로 기록한다',
  type: 'class',
  w: 1040,
  h: 620,
  classes: [
    { id: 'SCN', x: 40, y: 150, w: 420, name: 'PScene', stereo: '장면 데이터 · 메인 스레드 전용', color: 'green', members: ['-_meshes : 번호 → HSceneMesh', '-_cameras : 번호 → HSceneCamera', '+CreateMesh() · DestroyMesh()', '+SetMeshWorldMatrix() · SetMeshMaterial()', '+CreateCamera() · SetCamera()', '+GetMeshes()'] },
    { id: 'SR', x: 600, y: 150, w: 400, name: 'PSceneRenderer', stereo: '장면 렌더러', color: 'blue', members: ['-_gbufferTextures · _depthStencilTexture', '-_outputTexture · _displayTexture', '-_compositeMaterial · _displayMaterial', '+Initialize() · Resize(w, h)', '+Render(scene, cameraID)', '+GetDisplayTexture()'] },
    { id: 'MESH', x: 40, y: 380, w: 200, name: 'HSceneMesh', stereo: '메시 배치', color: 'green', stack: 1, members: ['Mesh : IMesh', 'Materials', 'WorldMatrix'] },
    { id: 'CAM', x: 260, y: 380, w: 200, name: 'HSceneCamera', stereo: '카메라', color: 'green', stack: 1, members: ['ViewMatrix · Position', 'FovY · NearZ · FarZ', '+SetLookAt()'] },
    { id: 'CMD', x: 600, y: 380, w: 400, name: 'IJGGraphicsCommand', stereo: '그리기 명령', color: 'purple' }
  ],
  nodes: [
    { id: 'O1', x: 40, y: 30, w: 200, h: 56, color: 'gray', title: 'PWorld', lines: ['GameFrameWorks 월드'] },
    { id: 'O2', x: 270, y: 30, w: 300, h: 56, color: 'gray', title: 'StaticMeshComponent · CameraComponent', lines: ['번호(HSceneMeshID · HSceneCameraID) 보관'] },
    { id: 'O3', x: 640, y: 30, w: 360, h: 56, color: 'gray', title: 'PGameViewport · JGDevScene', lines: ['렌더러 소유 · 결과 표시'] },
    { id: 'RES', x: 40, y: 540, w: 420, h: 56, color: 'gray', title: 'IMesh · IRawMaterial', lines: ['에셋의 GPU 리소스 — 참조만 함'] },
    { id: 'KEY', x: 600, y: 470, w: 400, h: 96, color: 'white', shape: 'note', title: '범례', lines: ['◇— 소유 · → 사용 · 참조', '겹친 카드 = 여러 개', '+ public · - private'] }
  ],
  edges: [
    { from: 'O1', to: 'SCN', fs: 'b', ts: 't', fo: 0, to_off: -110, start: 'diamond', end: 'none', label: '소유' },
    { from: 'O2', to: 'SCN', fs: 'b', ts: 't', fo: -0, to_off: 170, label: '번호로 생성 · 수정' },
    { from: 'O3', to: 'SR', fs: 'b', ts: 't', fo: 0, to_off: 20, start: 'diamond', end: 'none', label: '소유' },
    { from: 'SR', to: 'SCN', fs: 'l', ts: 'r', label: 'Render 시 읽기' },
    { from: 'SCN', to: 'MESH', fs: 'b', ts: 't', fo: -110, to_off: 0, start: 'diamond', end: 'none', label: '1 : N' },
    { from: 'SCN', to: 'CAM', fs: 'b', ts: 't', fo: 110, to_off: 0, start: 'diamond', end: 'none', label: '1 : N' },
    { points: [[140, 494], [140, 540]], label: '참조' },
    { from: 'SR', to: 'CMD', fs: 'b', ts: 't', label: '그리기 기록' }
  ]
};
