#include "PCH/PCH.h"

// miniaudio 0.11.25 구현(Source/ThirdParty/miniaudio — 퍼블릭 도메인 또는 MIT-0). 이 파일 하나에서만 구현을 넣는다.
// ogg 는 저장소의 stb_vorbis 1.22 를 붙인다: stb_vorbis 헤더 → miniaudio 구현 → stb_vorbis 구현 순(miniaudio 의 연결 방식).
// 바깥 코드의 경고는 끈다. miniaudio 설정 매크로(MA_NO_*)는 두지 않는다 — 다른 파일이 같은 헤더를 그대로 읽어 구조체 모양이 같아야 한다.
#pragma warning(push, 0)

#define STB_VORBIS_HEADER_ONLY
#include "stb/stb_vorbis.c"

#define MINIAUDIO_IMPLEMENTATION
#include "miniaudio/miniaudio.h"

#undef STB_VORBIS_HEADER_ONLY
#include "stb/stb_vorbis.c"

#pragma warning(pop)
