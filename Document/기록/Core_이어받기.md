# Core 이어받기 메모

갱신 2026-10-24 19:08. Engine-34 · Engine-44 는 검토 중(넘김). 맡은 일 없음 — Engine-38(Core API Reference)은 Engine-34 계획을 사용자가 확인하면 시작한다.

## 지금 일

| 일 | 단계 | 기다리는 상대 |
|---|---|---|
| API Reference 계획 · 꼴 견본(Engine-34) | 검토 중 — 계획 `Document/기록/Core/Engine-34_APIReference_계획_2026-10-24.md`, 견본 `Document/기록/Core/Engine-34_견본/`, 보고함 글 `Engine-34_보고함_다듬음_2026-10-24.md`(Document Writer 다듬음 + Engine-35 숫자 반영), 그림 `Saved/UIShots/engine34_wiki_layout.png` · `engine34_doc_layers.png` | 엔진 총괄 — 보고함에 올림. 사용자 — 결정 셋(위키 자리 A/B · 예시 확인 1/2 · 주석 확인 가/나, 권고 A · 1 · 가) |
| 위키 00. Core 재검토(Engine-44) | 검토 중 — 수정안 15개 `Document/기록/Core/Engine-44_00Core_재검토_2026-10-24.md`(문서는 안 고침) | 엔진 총괄 · 사용자(Q-004 묶음 보고) |
| Core API Reference · README · 문서 주석(Engine-38) | In-Wait(엔진 총괄이 걸어 둠) | Engine-34 사용자 확인 |

## 끝낸 일 (10-24)

| 일 | 결과 |
|---|---|
| 1단계 위키 00.Core 피드백 | `Temp/개편검토/Core_00.Core_피드백_2026-10-24.html` — 17개, 사용자 "반영 해줘"(H-068)로 글 반영 |
| 00. Core 글 · 그림 반영 | 위키 `02. 위키/JGEngine/00. Core.md`(새 0-1 · 0-2 절, 7절 Audio 뺌, 2단계 모듈 이름), 그림 원본 `Tools/Ops/wiki_svg/specs/Core/`(고침 12 · 새 2 — 30 · 31). 보관본 `Temp/개편검토/backup/` |
| Asset → Core 합병안(선택지 D) 사실 | GameFrameWork 세션에 보냄 · 공유 메모 core_notes.md. 사용자는 GameFrameWork 로 통째로(H-077) |

## 잠금 · 빌드 · 미커밋

- 메인 트리 잠금 · 빌드 · 시험 없음. 메인 트리 코드는 고치지 않았다.
- 작업 폴더 `Temp/wt/engine34`(main `c6dd99d`) — 견본 예시를 JGConsole 에 넣어 빌드 · 실행한 곳. Engine-34 결정 2 가 안 2 면 다시 쓰고, 아니면 지워도 된다(엔진 총괄 판단).
- 미커밋 문서: `Document/기록/Core/` 의 Engine-34 계획 · 견본 폴더 · 보고함 글, Engine-44 재검토, 이 메모. `Engine-34_보고함초안_2026-10-24.md` 는 옛 숫자 초안이라 커밋하지 않아도 된다.

## Engine-38 을 시작할 때

- 대상 · 공개 판단은 Engine-34 계획 1절(Core 공개 판단 표 포함)과 게임 프로젝트가 쓰는 엔진 API 목록(Engine-35, `Document/기록/DevOps/Engine-35_게임이쓰는엔진API_2026-10-24.md`).
- 게임은 `Core.h` 를 PCH 가 아니라 틀의 `<이름>Defines.h` 로 받는다. PCH 만 쓰는 엔진 프로그램에서 예시를 빌드하면 `Core.h` 를 넣어야 한다.
- 문서 주석은 `///` + `@param` · `@returns` 만(Visual Studio 툴팁 기본 태그). 주의 네 줄(스레드 · 수명 · 순서 · 오류)은 태그 없이.
- Engine-44 수정안 2 · 3 · 4 · 5 · 9 · 11 · 12 · 13 은 Reference 와 같이 고치면 한 번에 맞는다.

## 짚어 둔 코드 일 (일감 아님 — 지금 문제가 없어 손대지 않음)

- `HDelegate::BindSP` 가 `std::shared_ptr` 를 받는데 `CreateSP` 는 `PSharedPtr` — 부르면 컴파일되지 않음(부르는 곳 없음, `Misc/Delegate.h`)
- `JGDevelopUnit` 쓰는 곳 없음(`Misc/DevelopUnit.h`)
- `JGSTRUCT` · `JGINTERFACE` 는 JGHeaderTool 이 읽지 않음(`Object/ObjectDefines.h`)
- `GModuleGlobalSystem::ReconnectModule` 비어 있음(assert)
- Core 의 `JGCLASS`(JGObject) · `JGENUM`(EKeyCode)은 코드 생성 대상이 아니라 등록되지 않음 — JGHeaderTool 이 SharedLib 만 처리
