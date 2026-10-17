// 2-3. 엔진 종료 순서
// 위에서 아래로 한 줄. 초기화의 역순으로 해제한다.
const steps = [
  { id: 'A', h: 40, color: 'gray', title: '창 닫힘 → Update가 false 반환' },
  { id: 'B', h: 40, color: 'blue', title: 'GCoreSystem::Destroy' },
  { id: 'C', h: 40, color: 'blue', title: '창 · 창 콜백 해제' },
  { id: 'D', h: 50, color: 'green', title: '시스템 Destroy', lines: ['등록 역순으로 호출'] },
  { id: 'D1', h: 50, color: 'purple', title: '모듈이 등록한 시스템 먼저 종료', lines: ['오디오 · 에셋 DB 등'] },
  { id: 'D2', h: 50, color: 'purple', title: '모듈 시스템', lines: ['연결 역순으로 모듈별 ShutdownModule 호출'] },
  { id: 'E', h: 50, color: 'green', title: 'GC 추가 실행', lines: ['Flush'] },
  { id: 'F', h: 50, color: 'green', title: '베이스 시스템 해제', lines: ['메모리는 끝에서 두 번째, 로그는 마지막'] },
  { id: 'G', h: 50, color: 'purple', title: 'DLL 언로드', lines: ['FreeLibrary'] }
];
let y = 30;
const nodes = steps.map((s) => {
  const n = Object.assign({ x: 50, y, w: 300 }, s);
  y += s.h + 30;
  return n;
});
const edges = [];
for (let i = 1; i < nodes.length; i++) {
  edges.push({ from: nodes[i - 1].id, to: nodes[i].id });
}

module.exports = {
  file: 'Core_05_꺼질때',
  alt: '종료 — 창 닫힘 → GCoreSystem::Destroy → 창 콜백 해제 → 시스템 Destroy(등록 역순: 모듈이 등록한 시스템 → 모듈 ShutdownModule) → GC Flush → 베이스 시스템 해제 → DLL 언로드',
  type: 'flow',
  w: 400,
  h: y,
  nodes,
  edges
};
