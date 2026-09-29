# 아컴호러 카드 게임 — 기본판 레퍼런스

JGEngine 에 붙일 게임 모듈의 기준 자료입니다. 범위는 **오리지널 기본판(Core Set, 2016)** 입니다. 조사자 5명과 캠페인 「광신도의 밤」(Night of the Zealot)의 시나리오 3개이며, 확장은 넣지 않았습니다. 개정판(2020)은 같은 카드를 매수만 늘린 재판이라 매수 차이만 데이터에 붙였습니다.

## 요약

- 카드 183종(플레이어 103, 조우 80)을 한/영 병기 JSON(`data/cards.json`)으로 정리했고, 카드 이미지 229장(영문판, 24MB)을 받았습니다.
- 룰은 공식 참조 안내서(Rules Reference, 한국어판)를 옮기지 않고 구현 명세 형태로 요약했습니다(`01`). 용어·키워드 대조표는 `02` 입니다. 용어는 한국어판 공식 표기를 따릅니다(장막값, 능력 테스트, 전용 카드, 역할군, 주요목적/주요사건 등).
- 캠페인·시나리오 셋업, 혼돈 주머니, 장소 연결은 `06` 과 `data/scenarios.json` 에 있습니다.
- 한계: 조우 카드 본문은 ArkhamDB 에 한국어 번역이 없어 영어 원문을 씁니다(`ko: null`, 66장). 이미지도 영문판뿐입니다.

## 읽는 순서

| 순서 | 파일 | 내용 |
|---|---|---|
| 1 | [01_룰_요약.md](01_룰_요약.md) | 게임 흐름, 라운드 단계, 행동, 능력 테스트, 적, 피해·쓰러짐·탈락, 캠페인·덱 구성, 엔진 구현 체크리스트 |
| 2 | [02_용어_키워드.md](02_용어_키워드.md) | 한/영 용어, 키워드, 텍스트 아이콘, 특성 대조표(기본판 등장 수 포함) |
| 3 | [06_캠페인_시나리오.md](06_캠페인_시나리오.md) | 캠페인 구조, 혼돈 주머니, 시나리오별 셋업·주요목적/주요사건·장소·결말 |
| 4 | [03_카드_데이터_명세.md](03_카드_데이터_명세.md) | `cards.json` 필드, 텍스트 마크업, 제외·예외, 엔진 적재 메모 |
| 5 | [04_플레이어_카드.md](04_플레이어_카드.md) | 조사자 5명과 플레이어 카드 표(자동 생성) |
| 6 | [05_조우_카드.md](05_조우_카드.md) | 조우 세트 16개의 카드 표(자동 생성, 스포일러 포함) |

## 폴더 구성

```
Document/아컴호러/
├─ README.md, 01~06_*.md
├─ data/
│  ├─ cards.json        정규화 카드 데이터 183종 (한/영, 개정판 매수, 장소 연결 병합)
│  ├─ scenarios.json    캠페인·시나리오·장소 기호/연결
│  └─ raw/              ArkhamDB API 원본 (core·rcore × en·ko, packs)
├─ images/              카드 이미지 <코드>.png(앞면) / <코드>b.png(뒷면)
└─ tools/               update.sh, extract_raw.pl, build_cards.pl, gen_lists.pl
```

## 기본판 한눈에

| 항목 | 내용 |
|---|---|
| 조사자 | 로랜드 뱅크스(수호자), 데이지 워커(탐구자), "스키즈" 오'툴(무법자), 애그니스 베이커(신비주의자), 웬디 애덤스(생존자) |
| 플레이어 카드 | 103종. 기본판 121장 / 개정판 219장. 조사자 5, 전용 카드 10(전용 약점 5 포함), 기본 약점 8, 역할군·중립 80 |
| 조우 카드 | 80종 112장. 조우 세트 16개(시나리오 전용 3 + 공용 13) |
| 시나리오 | 회합(The Gathering) → 한밤의 가면(The Midnight Masks) → 지하 세계의 포식자(The Devourer Below) |
| 인원 | 룰은 1~4인. 기본판 1세트의 카드 풀은 2인 기준 |

## 출처

| 자료 | 위치 | 쓴 곳 |
|---|---|---|
| ArkhamDB 공개 API | `https://arkhamdb.com/api/public/cards/?encounter=1`, 한국어는 `ko.arkhamdb.com` | 카드 데이터 전부 |
| ArkhamDB 카드 이미지 | `https://arkhamdb.com/bundles/cards/<코드>.png` | `images/` |
| arkham-cards-data (ArkhamCards 앱 데이터) | `https://github.com/zzorba/arkham-cards-data` — `rules/ko`, `rules/en`, `campaigns/notz`, `i18n/ko` | 룰 요약, 캠페인·시나리오 |
| SCED (Tabletop Simulator 모드) | `https://github.com/Chr1Z93/SCED-downloads` — `decomposed/campaign/Night of the Zealot/…/*.gmnotes` | 장소 기호·연결(공개 면 24장 전부와 미공개 면 8장을 이미지로 대조) |
| FFG 공식 문서 | Rules Reference, Learn to Play, Campaign Guide, FAQ v2.5(2026-02) PDF — 링크는 `01_룰_요약.md` §17 | 원문 확인용 |

## 저작권

카드 텍스트, 이미지, 룰 원문의 권리는 Fantasy Flight Games(한국어판은 코리아보드게임즈)에 있습니다. 이 폴더는 내부 개발 참고용입니다. 외부 배포나 상용 게임에 쓰려면 라이선스가 필요합니다. 룰과 시나리오 문서는 원문을 옮기지 않고 구현 관점으로 요약했습니다.

## 갱신·확장

- `bash tools/update.sh` 를 실행하면 ArkhamDB 에서 데이터를 다시 받아 `data/cards.json` 과 `04`·`05` 를 다시 만듭니다. `--images` 를 붙이면 빠진 이미지만 추가로 받습니다.
- `04`·`05` 는 생성 문서이므로 직접 고치지 말고 `tools/gen_lists.pl` 이나 데이터를 고치세요.
- `06` 과 `data/scenarios.json` 은 캠페인 가이드 데이터(ArkhamCards)와 SCED 에서 한 번 뽑아 만든 뒤 손으로 관리합니다. 장소 연결을 고쳤다면 `perl tools/build_cards.pl .` → `perl tools/gen_lists.pl .` 로 `cards.json` 과 `05` 에 다시 병합하세요.
- 확장을 추가할 때는 `extract_raw.pl` 의 팩 목록에 팩 코드를 더하고, `build_cards.pl` 의 `pack` 을 배열로 바꾸면 됩니다. 확장 전용 키워드는 `01_룰_요약.md` 의 "코어 밖" 목록에서 시작하면 됩니다.
- 나중에 볼 것: 금지 목록(Taboo), 공식 FAQ 판정(arkham-cards-data `faq/ko/core.json`), 코어 셋 2026(Chapter 2)과 차이.
