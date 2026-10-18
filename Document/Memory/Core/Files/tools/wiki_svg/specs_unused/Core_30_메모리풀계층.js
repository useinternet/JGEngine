// 2-1. 메모리 풀 계층 — 스레드 → HMemoryChunk → HSizeClass → HPage → 블록, 각 단계 1 : N(사용자 피드백 10-18 08:55 "1대 다 구조를 다이어그램으로")
// 코드: Core/Memory/MemoryPool.h(HMemoryPoolConfig ClassCount 18 · PageBytes 64KB), MemoryPool.cpp(HMemoryChunk · HSizeClass · HPage · BlocksPerPage)
const COL = { th: 30, ch: 220, sc: 430, pg: 640, bl: 850 };
const W = { th: 140, ch: 170, sc: 170, pg: 170, bl: 160 };
module.exports = {
  file: 'Core_30_메모리풀계층',
  alt: '메모리 풀 계층 — 스레드마다 HMemoryChunk 하나, HMemoryChunk 마다 크기별 HSizeClass 18개, HSizeClass 마다 64KB HPage 여러 개, HPage 마다 같은 크기 블록 여러 개로 이어지는 1 : N 구조',
  type: 'flow',
  w: 1040,
  h: 470,
  nodes: [
    // 스레드
    { id: 'T1', x: COL.th, y: 110, w: W.th, h: 50, color: 'gray', title: '스레드 A', lines: ['할당을 호출한 스레드'] },
    { id: 'T2', x: COL.th, y: 380, w: W.th, h: 50, color: 'gray', title: '스레드 B' },
    // 청크
    { id: 'C1', x: COL.ch, y: 110, w: W.ch, h: 50, color: 'blue', title: 'HMemoryChunk', lines: ['스레드 A 전용'] },
    { id: 'C2', x: COL.ch, y: 380, w: W.ch, h: 50, color: 'blue', title: 'HMemoryChunk', lines: ['스레드 B 전용'] },
    // 크기 구간
    { id: 'S1', x: COL.sc, y: 40, w: W.sc, h: 40, color: 'green', title: 'HSizeClass 8B' },
    { id: 'S2', x: COL.sc, y: 115, w: W.sc, h: 40, color: 'green', title: 'HSizeClass 64B' },
    { id: 'S3', x: COL.sc, y: 190, w: W.sc, h: 40, color: 'white', title: '… (16B ~ 512KB)' },
    { id: 'S4', x: COL.sc, y: 265, w: W.sc, h: 40, color: 'green', title: 'HSizeClass 1MB' },
    { id: 'S5', x: COL.sc, y: 385, w: W.sc, h: 40, color: 'green', title: 'HSizeClass × 18', small: true },
    // 페이지
    { id: 'P1', x: COL.pg, y: 70, w: W.pg, h: 50, color: 'yellow', title: 'HPage #1', lines: ['64KB · 64B 블록 묶음'] },
    { id: 'P2', x: COL.pg, y: 150, w: W.pg, h: 50, color: 'yellow', title: 'HPage #2', lines: ['필요하면 추가'] },
    { id: 'P3', x: COL.pg, y: 230, w: W.pg, h: 40, color: 'white', title: '…' },
    // 블록
    { id: 'B1', x: COL.bl, y: 40, w: W.bl, h: 36, color: 'orange', title: '블록 64B' },
    { id: 'B2', x: COL.bl, y: 95, w: W.bl, h: 36, color: 'orange', title: '블록 64B' },
    { id: 'B3', x: COL.bl, y: 150, w: W.bl, h: 36, color: 'white', title: '… (64B 면 약 800개)' }
  ],
  texts: [
    { x: COL.th + W.th / 2, y: 22, text: '스레드', bold: true, size: 13, color: '#2C2C2A' },
    { x: COL.ch + W.ch / 2, y: 22, text: '청크', bold: true, size: 13, color: '#2C2C2A' },
    { x: COL.sc + W.sc / 2, y: 22, text: '크기 구간', bold: true, size: 13, color: '#2C2C2A' },
    { x: COL.pg + W.pg / 2, y: 22, text: '페이지', bold: true, size: 13, color: '#2C2C2A' },
    { x: COL.bl + W.bl / 2, y: 22, text: '블록', bold: true, size: 13, color: '#2C2C2A' },
    { x: (COL.th + W.th + COL.ch) / 2, y: 22, text: '1 : 1', size: 12, color: '#A32D2D', bold: true },
    { x: (COL.ch + W.ch + COL.sc) / 2, y: 22, text: '1 : 18', size: 12, color: '#A32D2D', bold: true },
    { x: (COL.sc + W.sc + COL.pg) / 2, y: 22, text: '1 : N', size: 12, color: '#A32D2D', bold: true },
    { x: (COL.pg + W.pg + COL.bl) / 2, y: 22, text: '1 : N', size: 12, color: '#A32D2D', bold: true }
  ],
  edges: [
    { from: 'T1', to: 'C1', fs: 'r', ts: 'l' },
    { from: 'T2', to: 'C2', fs: 'r', ts: 'l' },
    { from: 'C1', to: 'S1', fs: 'r', ts: 'l', mx: 410 },
    { from: 'C1', to: 'S2', fs: 'r', ts: 'l', mx: 410 },
    { from: 'C1', to: 'S3', fs: 'r', ts: 'l', mx: 410 },
    { from: 'C1', to: 'S4', fs: 'r', ts: 'l', mx: 410 },
    { from: 'C2', to: 'S5', fs: 'r', ts: 'l' },
    { from: 'S2', to: 'P1', fs: 'r', ts: 'l', mx: 620 },
    { from: 'S2', to: 'P2', fs: 'r', ts: 'l', mx: 620 },
    { from: 'S2', to: 'P3', fs: 'r', ts: 'l', mx: 620 },
    { from: 'P1', to: 'B1', fs: 'r', ts: 'l', mx: 830 },
    { from: 'P1', to: 'B2', fs: 'r', ts: 'l', mx: 830 },
    { from: 'P1', to: 'B3', fs: 'r', ts: 'l', mx: 830 }
  ]
};
