#!/usr/bin/env bash
# 빌드 속도 측정(DevOps BS-0, 2026-10-02): worktree 에서 MSBuild 를 돌리고 벽시계 시간 · 로그를 남긴다.
# 사용: BS_WT=<worktree(Git Bash 경로)> BS_LOG=<로그 폴더> run_build.sh <tag> [Build|Rebuild] [MSBuild 추가 인자...]
#   cl.exe · link.exe 가 읽는 환경 변수는 호출자가 정한다. 예: CL=/Bt+ (파일별 컴파일 시간) LINK=/time (링크 시간)
#   예: CL=/Bt+ LINK=/time BS_WT=/c/Develop/JGEngine/Temp/bs_wt BS_LOG=/c/bslogs bash run_build.sh r2 Rebuild -p:UseMultiToolTask=true -p:EnforceProcessCountAcrossBuilds=true
# 주의: worktree 가 %TEMP% 아래면 MSBuild 파일 추적이 꺼져(MSB8029) 증분 빌드 측정이 틀린다. 전체 빌드만 잰다.
TAG="$1"; shift
TARGET="${1:-Build}"; shift
WT="${BS_WT:?BS_WT=<worktree> 필요}"
LOG="${BS_LOG:-$PWD/bs_logs}"
MSB="${BS_MSBUILD:-/c/Program Files/Microsoft Visual Studio/2022/Community/MSBuild/Current/Bin/MSBuild.exe}"
mkdir -p "$LOG"
LOGW=$(cygpath -w "$LOG")

cd "$WT" || exit 1
start=$(date +%s%N)
echo "start $(date +%H:%M:%S) tag=$TAG target=$TARGET CL='${CL:-}' _CL_='${_CL_:-}' LINK='${LINK:-}' _LINK_='${_LINK_:-}' extra='$*'" > "$LOG/$TAG.summary.txt"
MSYS_NO_PATHCONV=1 env -u NoDefaultCurrentDirectoryInExePath "$MSB" JGEngine.sln -nologo -m -nodeReuse:false \
	-t:"$TARGET" -p:Configuration=DevelopEngine -p:Platform=x64 "$@" \
	-v:minimal "-flp:logfile=$LOGW\\$TAG.log;verbosity=normal;performancesummary" > "$LOG/$TAG.console.txt" 2>&1
code=$?
end=$(date +%s%N)
secs=$(awk -v s="$start" -v e="$end" 'BEGIN { printf "%.1f", (e - s) / 1e9 }')
echo "end $(date +%H:%M:%S) exit=$code wall_s=$secs errors=$(grep -cE ' error [A-Z]+[0-9]+' "$LOG/$TAG.log")" >> "$LOG/$TAG.summary.txt"
cat "$LOG/$TAG.summary.txt"
