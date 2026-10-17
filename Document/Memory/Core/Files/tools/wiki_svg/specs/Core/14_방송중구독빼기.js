// 6-2. 브로드캐스트 중 구독 해제
module.exports = {
  file: 'Core_14_방송중구독빼기',
  alt: '브로드캐스트 중 구독 해제 — BroadCast 시작 시 잠금 카운트가 1 증가하고, 구독 1이 구독 3을 Remove하면 핸들만 무효로 표시되어 구독 3은 건너뛰며, BroadCast 종료(잠금 카운트 0) 후 가장 바깥 브로드캐스트가 끝날 때 무효 항목을 일괄 제거한다',
  type: 'flow',
  w: 400,
  h: 660,
  nodes: [
    { id: 'A', x: 40, y: 20, w: 320, h: 50, color: 'blue', title: 'BroadCast 시작', lines: ['잠금 카운트 +1'] },
    { id: 'B', x: 40, y: 100, w: 320, h: 50, color: 'orange', title: '구독 1 실행' },
    { id: 'C', x: 40, y: 180, w: 320, h: 50, color: 'orange', title: '구독 1 내부에서 구독 3 Remove' },
    { id: 'D', x: 40, y: 260, w: 320, h: 50, color: 'green', title: '구독 3', lines: ['핸들만 무효로 표시, 목록 항목은 유지'] },
    { id: 'E', x: 40, y: 340, w: 320, h: 50, color: 'orange', title: '구독 2 실행' },
    { id: 'F', x: 40, y: 420, w: 320, h: 50, color: 'green', title: '구독 3', lines: ['무효 핸들이므로 건너뜀'] },
    { id: 'G', x: 40, y: 500, w: 320, h: 50, color: 'blue', title: 'BroadCast 종료', lines: ['잠금 카운트 0'] },
    { id: 'H', x: 40, y: 580, w: 320, h: 50, color: 'green', title: '가장 바깥 브로드캐스트 종료 시 무효 항목 일괄 삭제' }
  ],
  edges: [
    { from: 'A', to: 'B' },
    { from: 'B', to: 'C' },
    { from: 'C', to: 'D' },
    { from: 'D', to: 'E' },
    { from: 'E', to: 'F' },
    { from: 'F', to: 'G' },
    { from: 'G', to: 'H' }
  ]
};
