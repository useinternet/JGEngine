// 2-1. GMemoryGlobalSystem과 HMemoryPool — 메모리 시스템 전체 구조 + 1 : N 계층(사용자 피드백 10-18 09:08 "합쳐서 해줘. 자연스럽게")
// 왼쪽 세로줄 = 소유 사슬(메모리 시스템 → 풀 → 청크 → 페이지 풀 → 페이지 → 블록), 겹친 카드 · 빨간 표시 = 여러 개(× 스레드 수 · × 18 · × N)
// 코드: Core/Memory/Memory.h(GMemoryGlobalSystem · PSharedPtr · PWeakPtr · HMemoryControlBlock), MemoryPool.h/.cpp(HMemoryPool · HMemoryChunk · HPagePool · HPage · HMemoryHeader), Allocator.h(HAllocator)
module.exports = {
  file: 'Core_25_메모리시스템구조',
  alt: 'GMemoryGlobalSystem과 HMemoryPool — 메모리 시스템이 HMemoryPool 을 소유하고, 풀 아래로 HMemoryChunk(스레드마다 하나) → HPagePool(청크마다 18개) → HPage(여러 개) → 블록(여러 개)이 1 : N 으로 이어지며, 큰 요청은 대형 블록 캐시가, 객체 참조는 제어 블록을 공유하는 PSharedPtr · PWeakPtr 가 맡는다',
  type: 'class',
  w: 1000,
  h: 1050,
  classes: [
    { id: 'GMS', x: 40, y: 30, w: 380, name: 'GMemoryGlobalSystem', stereo: '메모리 시스템 · 베이스 시스템 2번', color: 'green', members: ['+Allocate<T>() : PSharedPtr<T>', '+Wrap(this) · Flush()', '#Update()  // GC + 풀 Tick', '-AllocatedMemoryBlocks : 객체 → 제어 블록', '-AllocatedMemoryBlockQueue : GC 대기열', '-MemoryPool : HMemoryPool'] },
    { id: 'HMP', x: 40, y: 250, w: 380, name: 'HMemoryPool', stereo: '메모리 풀', color: 'blue', members: ['+Allocate(size, align) · Deallocate(ptr)', '+Tick()  // 10초 미사용 페이지 · 캐시 반환', '-Chunks[] : 스레드별 청크', '-LargeCache : 대형 블록 캐시'] },
    { id: 'HMC', x: 40, y: 446, w: 380, name: 'HMemoryChunk', stereo: '스레드별 청크', color: 'blue', stack: 2, badge: '× 스레드 수', members: ['-Classes[18] : HPagePool', '-원격 반납 스택 : 다른 스레드 해제(lock-free)'] },
    { id: 'HPP', x: 40, y: 616, w: 380, name: 'HPagePool', stereo: '같은 크기 페이지 풀 · 8B~1MB', color: 'blue', stack: 2, badge: '× 18', members: ['BlockSize · BlocksPerPage', 'PartialHead : 빈 블록이 남은 페이지', 'EmptyHead : 완전히 빈 페이지(회수 대기)'] },
    { id: 'HPG', x: 40, y: 798, w: 380, name: 'HPage', stereo: '64KB 페이지', color: 'blue', stack: 2, badge: '× N', members: ['FreeHead : 이 페이지의 빈 블록 목록'] },
    { id: 'BLK', x: 40, y: 950, w: 380, name: 'HMemoryHeader + 데이터', stereo: '블록 · 페이지 안에서 모두 같은 크기', color: 'orange', stack: 2, badge: '× N' },
    { id: 'LC', x: 540, y: 290, w: 300, name: 'HLargeCacheNode', stereo: '대형 블록 캐시', color: 'yellow', members: ['1MB 초과 · 정렬 16B 초과 요청', '해제 시 보관 후 재사용'] },
    { id: 'SP', x: 540, y: 30, w: 240, name: 'PSharedPtr<T> · PWeakPtr<T>', stereo: '강참조 · 약참조', color: 'purple', members: ['+Pin()  // 약참조 → 강참조'] },
    { id: 'CB', x: 810, y: 30, w: 170, name: 'HMemoryControlBlock', stereo: '참조 수', color: 'purple', members: ['RefCount', 'WeakCount'] },
    { id: 'IMO', x: 540, y: 170, w: 240, name: 'IMemoryObject', stereo: '관리 객체의 루트(2-4)', color: 'purple' },
    { id: 'HA', x: 540, y: 470, w: 300, name: 'HAllocator<T>', stereo: '엔진 컨테이너 할당자', color: 'orange', members: ['HList · HHashMap · HMap · HSet'] }
  ],
  nodes: [
    { id: 'KEY', x: 540, y: 620, w: 300, h: 110, color: 'white', shape: 'note', title: '범례', lines: ['◇— 소유 · → 반환 · 참조 · 사용', '겹친 카드 · 빨간 글 = 여러 개', '+ public · # protected · - private'] }
  ],
  edges: [
    { from: 'GMS', to: 'HMP', fs: 'b', ts: 't', start: 'diamond', end: 'none', label: '소유' },
    { from: 'HMP', to: 'HMC', fs: 'b', ts: 't', start: 'diamond', end: 'none', label: '1 : 스레드 수 (스레드마다 청크 하나)', lw: 260 },
    { from: 'HMC', to: 'HPP', fs: 'b', ts: 't', start: 'diamond', end: 'none', label: '1 : 18 (크기마다 하나, 고정)', lw: 260 },
    { from: 'HPP', to: 'HPG', fs: 'b', ts: 't', start: 'diamond', end: 'none', label: '1 : N (필요할 때 64KB 추가)', lw: 260 },
    { from: 'HPG', to: 'BLK', fs: 'b', ts: 't', start: 'diamond', end: 'none', label: '1 : N (64B 면 약 800개)', lw: 260 },
    { from: 'HMP', to: 'LC', fs: 'r', ts: 'l', fo: -30, to_off: -15, start: 'diamond', end: 'none', label: '큰 요청' },
    { from: 'GMS', to: 'SP', fs: 'r', ts: 'l', fo: -45, to_off: 0, label: 'Allocate<T>() 반환' },
    { from: 'SP', to: 'CB', fs: 'r', ts: 'l', fo: 0, to_off: -9, label: '공유' },
    { from: 'SP', to: 'IMO', fs: 'b', ts: 't', label: '가리키는 객체' },
    { from: 'HA', to: 'HMP', fs: 'l', ts: 'r', fo: 0, to_off: 48, mx: 480, label: '사용', lx: 480, ly: 448 }
  ]
};
