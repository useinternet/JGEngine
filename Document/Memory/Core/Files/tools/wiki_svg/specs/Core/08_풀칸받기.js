// 4-1. 메모리 풀 — 블록 할당 경로
module.exports = {
  file: 'Core_08_풀칸받기',
  alt: '메모리 풀 할당 구조 — 할당 요청은 크기와 정렬에 따라 스레드 청크(HPagePool → HPage → 블록) 또는 대형 블록으로 분기하며, 다른 스레드의 반납과 메인 스레드 Tick이 이를 보조함',
  type: 'flow',
  w: 960,
  h: 740,
  nodes: [
    { id: 'REQ', x: 370, y: 20, w: 220, h: 60, color: 'gray', title: '할당 요청', lines: ['크기 · 정렬'] },
    { id: 'Q', x: 360, y: 130, w: 240, h: 80, color: 'white', shape: 'diamond', lines: ['크기 1MB 이하이며', '정렬 16 이하인가'] },
    { id: 'LG', x: 680, y: 130, w: 250, h: 80, color: 'blue', title: 'HLargeHeader', lines: ['대형 블록 · 헤더 32B', '해제 시 HLargeCacheNode로 보관 후 재사용'] },
    { id: 'CH', x: 360, y: 280, w: 240, h: 60, color: 'blue', title: 'HMemoryChunk', lines: ['호출 스레드의 청크 · 스레드별 1개'] },
    { id: 'FREE', x: 30, y: 290, w: 190, h: 40, color: 'gray', title: '다른 스레드가 해제한 블록' },
    { id: 'SZ', x: 360, y: 395, w: 240, h: 70, color: 'green', title: 'HPagePool', lines: ['크기에 맞는 페이지 풀 선택(18개 중 하나)', '8B · 16B · … · 1MB'] },
    { id: 'TICK', x: 30, y: 390, w: 200, h: 80, color: 'gray', title: '메인 스레드 Tick', lines: ['10초간 사용되지 않은', '빈 페이지를 시스템에 반환'] },
    { id: 'PG', x: 360, y: 520, w: 240, h: 80, color: 'white', shape: 'diamond', lines: ['빈 블록이 있는', '페이지가 있는가'] },
    { id: 'GROW', x: 680, y: 525, w: 250, h: 70, color: 'green', title: 'HPage', lines: ['64KB 페이지 추가 할당', '고갈 없음'] },
    { id: 'BLK', x: 360, y: 660, w: 240, h: 60, color: 'yellow', title: 'HMemoryHeader + 데이터', lines: ['블록 1개 · 헤더 16B'] }
  ],
  edges: [
    { from: 'REQ', to: 'Q' },
    { from: 'Q', to: 'CH', label: '예' },
    { from: 'Q', to: 'LG', fs: 'r', ts: 'l', label: '아니오' },
    { from: 'CH', to: 'SZ' },
    { from: 'SZ', to: 'PG' },
    { from: 'PG', to: 'BLK', label: '예' },
    { from: 'PG', to: 'GROW', fs: 'r', ts: 'l', label: '아니오' },
    { from: 'GROW', to: 'BLK', fs: 'b', ts: 'r' },
    { from: 'FREE', to: 'CH', fs: 'r', ts: 'l', dash: true, label: 'lock-free 반납 스택' },
    { from: 'TICK', to: 'SZ', fs: 'r', ts: 'l', dash: true }
  ]
};
