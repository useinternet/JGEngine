// 3-1. GObjectGlobalSystem과 JGClass — 리플렉션 시스템이 클래스 · 열거형 정보를 타입별로 보관하고, 생성 코드가 등록한다
// 코드: Core/Object/ObjectGlobalSystem.h/.cpp(GObjectGlobalSystem · codeGen), ObjectGlobals.h(JGField · JGClass · JGProperty · JGFunction · JGEnum), JGObject.h, JGType.h
module.exports = {
  file: 'Core_26_리플렉션구조',
  alt: 'GObjectGlobalSystem과 JGClass — GObjectGlobalSystem은 이름 · 타입별로 JGClass와 JGEnum, 생성 함수를 보관하고, JGHeaderTool이 생성한 Engine_CodeGenerate가 클래스를 등록하며, JGClass는 JGField를 상속해 속성 · 함수 정보를 가진다',
  type: 'class',
  w: 1060,
  h: 580,
  classes: [
    { id: 'GOS', x: 30, y: 30, w: 360, name: 'GObjectGlobalSystem', stereo: '리플렉션 시스템 · 베이스 시스템', color: 'green', members: ['-_typeMap : 이름 → JGType', '-_classMap : JGType → JGClass', '-_enumMap : JGType → JGEnum', '-_createObjectFuncPool : JGType → 생성 함수', '+GetStaticClass(type) · NewObject(type)', '+RegisterJGClass() · RegisterJGEnum()', '-codeGen()  // DLL별 Engine_CodeGenerate 호출'] },
    { id: 'JGE', x: 450, y: 30, w: 250, name: 'JGEnum', stereo: '열거형 정보', color: 'purple', members: ['ElementNames · 값 ↔ 이름 변환'] },
    { id: 'JGC', x: 450, y: 150, w: 250, name: 'JGClass', stereo: '클래스 정보', color: 'purple', members: ['Type : JGType', 'ParentTypeSet · ChildTypeSet', 'MetaData : JGMeta'] },
    { id: 'JGF', x: 760, y: 150, w: 270, name: 'JGField', stereo: '속성 · 함수 목록', color: 'purple', members: ['Properties : JGProperty 목록', 'Functions : JGFunction 목록', '+FindProperty(name)', '+FindFunction(name)'] },
    { id: 'JGP', x: 760, y: 330, w: 128, name: 'JGProperty', stereo: '속성 정보', color: 'purple' },
    { id: 'JGFN', x: 902, y: 330, w: 128, name: 'JGFunction', stereo: '함수 정보', color: 'purple' },
    { id: 'JGO', x: 450, y: 330, w: 250, name: 'JGObject', stereo: '리플렉션 객체의 기반 클래스', color: 'blue', members: ['IMemoryObject · IJsonable 상속', '+GetClass() · GetType()', '+WriteJson() · ReadJson()'] },
    { id: 'USR', x: 450, y: 500, w: 250, name: 'JGActor 등', stereo: 'JGCLASS 마커가 붙은 클래스', color: 'blue' }
  ],
  nodes: [
    { id: 'GEN', x: 30, y: 260, w: 360, h: 96, color: 'yellow', title: 'Engine_CodeGenerate(GObjectGlobalSystem*)', lines: ['모듈 DLL마다 하나 · JGHeaderTool이 생성', '클래스마다 RegisterJGClass 호출', '클래스 정보 · 속성 바인딩 · 생성 함수 등록'] },
    { id: 'KEY', x: 30, y: 420, w: 360, h: 100, color: 'white', shape: 'note', title: '범례', lines: ['◇— 보관 · —▷ 상속 · → 호출 · 조회 · - - → 등록', '+ public · # protected · - private', 'JGClass, JGField, JGEnum 등 메타 클래스도 JGObject 상속'] }
  ],
  edges: [
    { from: 'GOS', to: 'JGE', fs: 'r', ts: 'l', fo: -54, to_off: 0, start: 'diamond', end: 'none', label: '보관' },
    { from: 'GOS', to: 'JGC', fs: 'r', ts: 'l', fo: 75, to_off: -9, start: 'diamond', end: 'none', label: '보관' },
    { from: 'JGC', to: 'JGF', fs: 'r', ts: 'l', fo: 0, to_off: -9, end: 'triangle' },
    { from: 'JGF', to: 'JGP', fs: 'b', ts: 't', fo: -71, to_off: 0, start: 'diamond', end: 'none' },
    { from: 'JGF', to: 'JGFN', fs: 'b', ts: 't', fo: 71, to_off: 0, start: 'diamond', end: 'none' },
    { from: 'JGO', to: 'JGC', fs: 't', ts: 'b', label: 'GetClass()' },
    { from: 'USR', to: 'JGO', fs: 't', ts: 'b', end: 'triangle' },
    { from: 'GEN', to: 'GOS', fs: 't', ts: 'b', dash: true, label: 'RegisterJGClass' },
    { points: [[450, 522], [420, 522], [420, 308], [390, 308]], dash: true, label: 'JGHeaderTool이 생성', lx: 420, ly: 388, lpos: 'l' }
  ]
};
