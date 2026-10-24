// HMulticastDelegate와 HDelegate — 델리게이트 클래스 구조
module.exports = {
  file: 'Core_13_대리자짜임',
  alt: 'HMulticastDelegate와 HDelegate — HMulticastDelegate는 구독마다 HDelegate를 하나씩 포함하고 Add 계열 함수는 HDelegateHandle을 반환하며, HDelegate는 IDelegateInstance(정적 · 원시 포인터 · 람다 · 약참조 델리게이트)를 포함한다',
  type: 'class',
  w: 850,
  h: 640,
  classes: [
    { id: 'HM', x: 310, y: 20, w: 200, name: 'HMulticastDelegate', color: 'blue', members: ['+AddRaw()', '+AddStatic()', '+AddLambda()', '+AddSP()', '+Remove(handle)', '+RemoveObject(owner)', '+RemoveAll()', '+BroadCast()'] },
    { id: 'HH', x: 665, y: 80, w: 170, name: 'HDelegateHandle', color: 'yellow', members: ['+IsValid()'] },
    { id: 'HD', x: 330, y: 300, w: 160, name: 'HDelegate', color: 'blue', members: ['+Execute()', '+IsBound()'] },
    { id: 'IDI', x: 320, y: 460, w: 180, name: 'IDelegateInstance', color: 'green', members: ['+Execute()'] },
    { id: 'PS', x: 30, y: 580, w: 160, name: 'PStaticDelegate', color: 'green' },
    { id: 'PR', x: 230, y: 580, w: 160, name: 'PRawDelegate', color: 'green' },
    { id: 'PL', x: 430, y: 580, w: 160, name: 'PLambdaDelegate', color: 'green' },
    { id: 'PSP', x: 630, y: 580, w: 160, name: 'PSPDelegate', color: 'green' }
  ],
  edges: [
    { from: 'HM', to: 'HD', fs: 'b', ts: 't', start: 'diamond', end: 'none', label: '구독마다 1개' },
    { from: 'HM', to: 'HH', fs: 'r', ts: 'l', to_off: 3, label: 'Add 계열 함수가 반환' },
    { from: 'HD', to: 'IDI', fs: 'b', ts: 't', start: 'diamond', end: 'none', label: '실제 호출 대상 함수' },
    { from: 'PS', to: 'IDI', fs: 't', ts: 'b', my: 550, end: 'triangle' },
    { from: 'PR', to: 'IDI', fs: 't', ts: 'b', my: 550, end: 'triangle' },
    { from: 'PL', to: 'IDI', fs: 't', ts: 'b', my: 550, end: 'triangle' },
    { from: 'PSP', to: 'IDI', fs: 't', ts: 'b', my: 550, end: 'triangle' }
  ]
};
