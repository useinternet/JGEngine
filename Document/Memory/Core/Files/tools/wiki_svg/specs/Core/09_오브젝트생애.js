// 4-2. 오브젝트 수명 주기
module.exports = {
  file: 'Core_09_오브젝트생애',
  alt: '객체 수명 주기 — 게임 코드, 메모리 시스템, 메모리 풀, 객체 사이의 할당, 강참조 · 약참조, 다음 프레임 GC에 의한 파괴, 블록 반납 순서',
  type: 'seq',
  colW: 240,
  participants: [
    { id: 'G', label: '게임 코드', color: 'orange' },
    { id: 'MS', label: 'GMemoryGlobalSystem', sub: '메모리 시스템', color: 'blue' },
    { id: 'P', label: 'HMemoryPool', sub: '메모리 풀', color: 'green' },
    { id: 'O', label: '객체', color: 'yellow' }
  ],
  steps: [
    { from: 'G', to: 'MS', text: 'Allocate<T>' },
    { from: 'MS', to: 'P', text: '블록 할당' },
    { from: 'MS', to: 'O', text: '객체 생성 + 제어 블록' },
    { from: 'MS', to: 'G', text: 'PSharedPtr 반환 (강참조 1)', dash: true },
    { note: '복사 · 전달 시 강참조 수 증감', over: ['G'] },
    { note: 'PWeakPtr — 약참조, 수명과 무관', over: ['G'] },
    { note: '마지막 PSharedPtr 해제 — 강참조 0', over: ['G'] },
    { note: '다음 프레임 시작 시 GC', over: ['MS'] },
    { from: 'MS', to: 'O', text: '파괴 (소멸자 호출)' },
    { from: 'MS', to: 'P', text: '블록 반납' },
    { note: 'PWeakPtr Pin — 이후 실패', over: ['G'], color: 'red' }
  ]
};
