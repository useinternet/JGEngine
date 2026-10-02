#!/usr/bin/env bash
# 증분 빌드 시나리오(DevOps BS-0). worktree 는 %TEMP% 밖이어야 한다(파일 추적이 동작해야 헤더 수정이 잡힌다).
#   I1 변경 없음 / I2 GFW .cpp 1개 / I3 GFW 공용 헤더(GameFrameWorksDefines.h) / I4 Core 공용 헤더(CoreDefines.h)
# 사용: BS_WT=<worktree> BS_LOG=<로그 폴더> incremental_real.sh <prefix> [MSBuild 추가 인자...]  (먼저 같은 인자로 전체 빌드)
PREFIX="$1"; shift
HERE="$(cd "$(dirname "$0")" && pwd)"
RUN="$HERE/run_build.sh"
ncl() { powershell -NoProfile -Command "(Get-CimInstance Win32_Process -Filter \"Name='cl.exe'\" | Measure-Object).Count"; }

echo "other cl.exe before: $(ncl)"; bash "$RUN" "${PREFIX}_i1_noop" Build "$@" | tail -1
touch "$BS_WT/Source/Runtime/GameFrameWorks/Core/GameInstance.cpp"
echo "other cl.exe before: $(ncl)"; bash "$RUN" "${PREFIX}_i2_one_cpp" Build "$@" | tail -1
touch "$BS_WT/Source/Runtime/GameFrameWorks/Core/GameFrameWorksDefines.h"
echo "other cl.exe before: $(ncl)"; bash "$RUN" "${PREFIX}_i3_gfw_header" Build "$@" | tail -1
touch "$BS_WT/Source/Runtime/Core/CoreDefines.h"
echo "other cl.exe before: $(ncl)"; bash "$RUN" "${PREFIX}_i4_core_header" Build "$@" | tail -1
