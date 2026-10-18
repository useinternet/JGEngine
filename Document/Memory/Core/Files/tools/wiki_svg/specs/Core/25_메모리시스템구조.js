// 2-1. GMemoryGlobalSystem과 HMemoryPool — 메모리 시스템 전체 구조 + 1 : N 계층
// 사용자 피드백 10-18 09:08 "합쳐서 해줘", 09:17 "사소한 거 적지 말아 줘"(스레드 수 · 18 · 64KB 같은 세부 수치, 강참조 · 약참조 풀이 뺌),
// 09:18 2-2(할당 구조) 삭제 — 큰 요청 갈래만 이 그림의 HLargeHeader 로 옮김
// 왼쪽 세로줄 = 소유 사슬(메모리 시스템 → 풀 → 청크 → 페이지 풀 → 페이지 → 블록), 겹친 카드 = 여러 개
// 코드: Core/Memory/Memory.h(GMemoryGlobalSystem · PSharedPtr · PWeakPtr · HMemoryControlBlock), MemoryPool.h/.cpp(HMemoryPool · HMemoryChunk · HPagePool · HPage · HMemoryHeader · HLargeHeader), Allocator.h(HAllocator)
module.exports = {
  file: 'Core_25_메모리시스템구조',
  alt: 'GMemoryGlobalSystem과 HMemoryPool — GMemoryGlobalSystem이 HMemoryPool을 소유하고, HMemoryPool 아래로 HMemoryChunk → HPagePool → HPage → 블록이 1 : N으로 이어지며, 큰 요청은 HLargeHeader가 처리하고, Allocate<T>()가 반환한 PSharedPtr · PWeakPtr는 HMemoryControlBlock을 공유한다',
  type: 'class',
  w: 980,
  h: 970,
  classes: [
    { id: 'GMS', x: 40, y: 30, w: 340, name: 'GMemoryGlobalSystem', stereo: '메모리 시스템', color: 'green', members: ['+Allocate<T>() : PSharedPtr<T>', '#Update()  // GC', '-MemoryPool : HMemoryPool'] },
    { id: 'HMP', x: 40, y: 210, w: 340, name: 'HMemoryPool', stereo: '메모리 풀', color: 'blue', members: ['+Allocate(size, align) · Deallocate(ptr)', '-Chunks : HMemoryChunk[]'] },
    { id: 'HMC', x: 40, y: 380, w: 340, name: 'HMemoryChunk', stereo: '스레드별 청크', color: 'blue', stack: 2, members: ['-Classes : HPagePool[]'] },
    { id: 'HPP', x: 40, y: 544, w: 340, name: 'HPagePool', stereo: '같은 크기 블록의 페이지 풀', color: 'blue', stack: 2, members: ['PartialHead · EmptyHead : HPage 목록'] },
    { id: 'HPG', x: 40, y: 708, w: 340, name: 'HPage', stereo: '페이지', color: 'blue', stack: 2, members: ['FreeHead : 빈 블록 목록'] },
    { id: 'BLK', x: 40, y: 872, w: 340, name: 'HMemoryHeader + 데이터', stereo: '블록', color: 'orange', stack: 2 },
    { id: 'SP', x: 520, y: 30, w: 240, name: 'PSharedPtr<T> · PWeakPtr<T>', stereo: '스마트 포인터', color: 'purple' },
    { id: 'CB', x: 800, y: 30, w: 170, name: 'HMemoryControlBlock', stereo: '참조 수', color: 'purple', members: ['RefCount · WeakCount'] },
    { id: 'IMO', x: 520, y: 150, w: 240, name: 'IMemoryObject', stereo: '관리 객체의 루트', color: 'purple' },
    { id: 'LH', x: 520, y: 234, w: 240, name: 'HLargeHeader', stereo: '대형 블록', color: 'yellow' },
    { id: 'HA', x: 520, y: 330, w: 240, name: 'HAllocator<T>', stereo: '엔진 컨테이너 할당자', color: 'orange' }
  ],
  nodes: [
    { id: 'KEY', x: 520, y: 440, w: 300, h: 90, color: 'white', shape: 'note', title: '범례', lines: ['◇— 소유 · → 반환 · 참조 · 사용', '겹친 카드 = 여러 개', '+ public · # protected · - private'] }
  ],
  edges: [
    { from: 'GMS', to: 'HMP', fs: 'b', ts: 't', start: 'diamond', end: 'none', label: '소유' },
    { from: 'HMP', to: 'HMC', fs: 'b', ts: 't', start: 'diamond', end: 'none', label: '1 : N' },
    { from: 'HMC', to: 'HPP', fs: 'b', ts: 't', start: 'diamond', end: 'none', label: '1 : N' },
    { from: 'HPP', to: 'HPG', fs: 'b', ts: 't', start: 'diamond', end: 'none', label: '1 : N' },
    { from: 'HPG', to: 'BLK', fs: 'b', ts: 't', start: 'diamond', end: 'none', label: '1 : N' },
    { from: 'HMP', to: 'LH', fs: 'r', ts: 'l', fo: 1, to_off: 0, start: 'diamond', end: 'none', label: '큰 요청' },
    { from: 'GMS', to: 'SP', fs: 'r', ts: 'l', fo: -32, to_off: 0, label: 'Allocate<T>() 반환' },
    { from: 'SP', to: 'CB', fs: 'r', ts: 'l', fo: 0, to_off: -14, label: '공유' },
    { from: 'SP', to: 'IMO', fs: 'b', ts: 't', label: '가리키는 객체' },
    { from: 'HA', to: 'HMP', fs: 'l', ts: 'r', fo: 0, to_off: 30, mx: 450, label: '사용', lx: 450, ly: 318 }
  ]
};
