// 2-1. GMemoryGlobalSystem과 메모리 풀 — 메모리 시스템 전체 구조(사용자 피드백 21:57 "메모리 시스템 구조 설명부터")
// 코드: Core/Memory/Memory.h(GMemoryGlobalSystem · PSharedPtr · PWeakPtr · HMemoryControlBlock), MemoryPool.h/.cpp(HMemoryPool · HMemoryChunk · HSizeClass · HPage), Allocator.h(HAllocator)
module.exports = {
  file: 'Core_25_메모리시스템구조',
  alt: 'GMemoryGlobalSystem과 메모리 풀 — 메모리 시스템이 메모리 풀을 소유하고, 풀은 스레드별 청크(HSizeClass 18개 → 64KB HPage → 블록)와 대형 블록 캐시로 할당하며, 객체는 제어 블록을 공유하는 PSharedPtr · PWeakPtr로 참조하고 엔진 컨테이너는 HAllocator로 같은 풀을 사용한다',
  type: 'class',
  w: 1000,
  h: 930,
  classes: [
    { id: 'GMS', x: 40, y: 30, w: 360, name: 'GMemoryGlobalSystem', stereo: '메모리 시스템 · 베이스 시스템 2번', color: 'green', members: ['+Allocate<T>() : PSharedPtr<T>', '+Wrap(this) · Flush()', '#Update()  // GC + 풀 Tick', '-AllocatedMemoryBlocks : 객체 → 제어 블록', '-AllocatedMemoryBlockQueue : GC 대기열', '-MemoryPool : HMemoryPool'] },
    { id: 'HMP', x: 40, y: 260, w: 360, name: 'HMemoryPool', stereo: '메모리 풀', color: 'blue', members: ['+Allocate(size, align) · Deallocate(ptr)', '+Tick()  // 10초 미사용 페이지 · 캐시 반환', '-Chunks[] : 스레드별 청크', '-LargeCache : 대형 블록 캐시'] },
    { id: 'HMC', x: 40, y: 470, w: 360, name: 'HMemoryChunk', stereo: '스레드별 청크 · 호출 스레드마다 하나', color: 'blue', members: ['-Classes[18] : HSizeClass', '-원격 반납 스택 : 다른 스레드 해제(lock-free)'] },
    { id: 'HSC', x: 40, y: 620, w: 360, name: 'HSizeClass', stereo: '크기 클래스 · 8B~1MB 2의 거듭제곱 18개', color: 'blue', members: ['BlockSize · Stride(블록 + 헤더 16B)', 'BlocksPerPage : 페이지당 블록 수', 'PartialHead : 빈 블록이 남은 페이지', 'EmptyHead : 완전히 빈 페이지(회수 대기)'] },
    { id: 'HPG', x: 40, y: 810, w: 360, name: 'HPage', stereo: '64KB 페이지 · 같은 크기 블록 묶음', color: 'blue', members: ['FreeHead : 이 페이지의 빈 블록 목록', '블록 = HMemoryHeader(16B) + 데이터'] },
    { id: 'LC', x: 520, y: 290, w: 300, name: 'HLargeCacheNode', stereo: '대형 블록 캐시', color: 'yellow', members: ['1MB 초과 · 정렬 16B 초과 요청', '해제 시 보관 후 재사용'] },
    { id: 'SP', x: 520, y: 30, w: 240, name: 'PSharedPtr<T> · PWeakPtr<T>', stereo: '강참조 · 약참조', color: 'purple', members: ['+Pin()  // 약참조 → 강참조'] },
    { id: 'CB', x: 800, y: 30, w: 170, name: 'HMemoryControlBlock', stereo: '참조 수', color: 'purple', members: ['RefCount', 'WeakCount'] },
    { id: 'IMO', x: 520, y: 170, w: 240, name: 'IMemoryObject', stereo: '관리 객체의 루트(2-4)', color: 'purple' },
    { id: 'HA', x: 520, y: 470, w: 300, name: 'HAllocator<T>', stereo: '엔진 컨테이너 할당자', color: 'orange', members: ['HList · HHashMap · HMap · HSet'] }
  ],
  nodes: [
    { id: 'KEY', x: 520, y: 600, w: 300, h: 100, color: 'white', shape: 'note', title: '범례', lines: ['◇— 소유 · → 반환 · 참조 · 사용', '+ public · # protected · - private'] }
  ],
  edges: [
    { from: 'GMS', to: 'HMP', fs: 'b', ts: 't', start: 'diamond', end: 'none', label: '소유' },
    { from: 'HMP', to: 'HMC', fs: 'b', ts: 't', start: 'diamond', end: 'none', label: '스레드마다 청크 1개' },
    { from: 'HMC', to: 'HSC', fs: 'b', ts: 't', start: 'diamond', end: 'none', label: '크기마다 하나 · 18개' },
    { from: 'HSC', to: 'HPG', fs: 'b', ts: 't', start: 'diamond', end: 'none', label: '페이지 목록(Partial · Empty)' },
    { from: 'HMP', to: 'LC', fs: 'r', ts: 'l', fo: -30, to_off: -15, start: 'diamond', end: 'none', label: '큰 요청' },
    { from: 'GMS', to: 'SP', fs: 'r', ts: 'l', fo: -45, to_off: 0, label: 'Allocate<T>() 반환' },
    { from: 'SP', to: 'CB', fs: 'r', ts: 'l', fo: 0, to_off: -9, label: '공유' },
    { from: 'SP', to: 'IMO', fs: 'b', ts: 't', label: '가리키는 객체' },
    { from: 'HA', to: 'HMP', fs: 'l', ts: 'r', fo: 0, to_off: 48, mx: 460, label: '사용', lx: 460, ly: 448 }
  ]
};
