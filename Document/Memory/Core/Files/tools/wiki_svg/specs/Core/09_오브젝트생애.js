// 2-2. 객체 수명 주기 — 상태 5개(사용자 피드백 10-18 09:13 "이해하기 쉽게 다시", 09:17 "사소한 거 적지 말아 줘" — 강참조 · 약참조 풀이 · 종료 시 예외 뺌)
// 가운데 줄 = 상태(위 → 아래), 화살표 글 = 다음 상태로 가는 조건, 오른쪽 = 그 상태에서 PWeakPtr::Pin() 결과
// 코드: Core/Memory/Memory.h(PSharedPtr · PWeakPtr::Pin — RefCount 0 이면 실패, HMemoryControlBlock), Memory.cpp(GC)
const X = 40;
const W = 400;
module.exports = {
  file: 'Core_09_객체수명',
  alt: '객체 수명 주기 — 생성 → 사용 중(PSharedPtr가 남아 있음) → GC 대기(마지막 PSharedPtr 해제) → 파괴(다음 프레임 GC) → 해제 완료(마지막 PWeakPtr 해제), PWeakPtr::Pin()은 사용 중일 때만 성공',
  type: 'flow',
  w: 820,
  h: 620,
  nodes: [
    { id: 'S0', x: X, y: 20, w: W, h: 60, color: 'blue', title: '① 생성', lines: ['Allocate<T>()가 PSharedPtr를 반환'] },
    { id: 'S1', x: X, y: 150, w: W, h: 60, color: 'green', title: '② 사용 중', lines: ['PSharedPtr가 하나 이상 남아 있음'] },
    { id: 'S2', x: X, y: 280, w: W, h: 60, color: 'yellow', title: '③ GC 대기', lines: ['PSharedPtr가 모두 해제됨'] },
    { id: 'S3', x: X, y: 410, w: W, h: 60, color: 'red', title: '④ 파괴', lines: ['소멸자 호출 · 메모리를 HMemoryPool에 반납'] },
    { id: 'S4', x: X, y: 540, w: W, h: 60, color: 'gray', title: '⑤ 해제 완료', lines: ['HMemoryControlBlock 반납'] },
    { id: 'P1', x: 540, y: 155, w: 250, h: 50, color: 'green', title: 'PWeakPtr::Pin() 성공', lines: ['PSharedPtr 반환'] },
    { id: 'P2', x: 540, y: 285, w: 250, h: 50, color: 'red', title: 'PWeakPtr::Pin() 실패', lines: ['빈 포인터 반환'] }
  ],
  edges: [
    { from: 'S0', to: 'S1' },
    { from: 'S1', to: 'S2', label: '마지막 PSharedPtr 해제' },
    { from: 'S2', to: 'S3', label: '다음 프레임 GC' },
    { from: 'S3', to: 'S4', label: '마지막 PWeakPtr 해제' },
    { from: 'S1', to: 'P1', fs: 'r', ts: 'l', dash: true },
    { from: 'S2', to: 'P2', fs: 'r', ts: 'l', dash: true },
    { from: 'S3', to: 'P2', fs: 'r', ts: 'b', dash: true }
  ]
};
