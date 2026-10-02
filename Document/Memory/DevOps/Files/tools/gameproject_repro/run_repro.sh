#!/usr/bin/env bash
# 외부 GameProject(MyGame: Source/Game, Source/Editor)를 현재 JGHeaderTool / JGBuildTool 에 물렸을 때의 동작 재현.
# 엔진 트리는 읽기만 한다. 가짜 엔진 루트(모듈 json · 템플릿 · 툴 exe · premake 만 복사)를 <workDir>/engine 에 만든다.
# 사용법: bash run_repro.sh <engineRoot> <workDir> [toolTimeoutSeconds=90]
#   예) bash run_repro.sh /c/JG/JGEngine "$TEMP/gp_repro"
# 결과: <workDir>/result.txt (툴별 종료 코드 · 행 여부), pass{1,2}_*_stdout.txt, 생성된 lua / vcxproj 사본
# 툴이 CRT abort 대화상자로 멈추면 timeout 뒤 강제 종료하고 "HUNG" 으로 기록한다.
set -u
ENGINE="$1"
WORK="$2"
TIMEOUT="${3:-90}"
HERE="$(cd "$(dirname "$0")" && pwd)"
FAKE="$WORK/engine"
PROJ="$WORK/MyGame"
RESULT="$WORK/result.txt"

rm -rf "$WORK"
mkdir -p "$FAKE/Build/BatchFiles" "$FAKE/Bin/DevelopEngine" "$FAKE/Source/PCH" "$FAKE/Source/ThirdParty" "$FAKE/Source/Programs/JGBuildTool/Template" "$FAKE/Temp"

# 모듈 json 만 트리째 복사 (소스는 복사하지 않는다 → 엔진 쪽 PCH 삽입 · premake 글롭은 빈 결과)
MSYS_NO_PATHCONV=1 robocopy "$(cygpath -w "$ENGINE/Source")" "$(cygpath -w "$FAKE/Source")" "*.module.json" /S /XD ThirdParty /NFL /NDL /NJH /NJS /NP > /dev/null
cp "$ENGINE/Build/premake5.exe" "$FAKE/Build/"
cp "$ENGINE/Bin/DevelopEngine/JGBuildTool.exe" "$ENGINE/Bin/DevelopEngine/JGHeaderTool.exe" "$FAKE/Bin/DevelopEngine/"
cp "$ENGINE/Source/PCH/PCH.h" "$ENGINE/Source/PCH/PCH.cpp" "$FAKE/Source/PCH/"
cp "$ENGINE/Source/Programs/JGBuildTool/Template/BuildTemplate.lua" "$FAKE/Source/Programs/JGBuildTool/Template/"

# 사용자 요구 구조의 게임 프로젝트
cp -r "$HERE/MyGame" "$PROJ"
mkdir -p "$PROJ/Bin" "$PROJ/Content" "$PROJ/Temp"
P="$(cygpath -m "$PROJ")"
sed "s#@PROJECT@#$P#" "$HERE/buildtool_arguments.json.in" > "$FAKE/Source/Programs/JGBuildTool/buildtool_arguments.json"
sed "s#@PROJECT@#$P#" "$HERE/headertool_arguments.json.in" > "$FAKE/Source/Programs/JGHeaderTool/headertool_arguments.json"

run_tool()
{
	local tag="$1"
	local exe="$2"
	"$exe" > "$WORK/${tag}_stdout.txt" 2>&1 &
	local pid=$!
	local waited=0
	while kill -0 "$pid" 2>/dev/null
	do
		if [ "$waited" -ge "$TIMEOUT" ]
		then
			local winpid
			winpid="$(cat /proc/$pid/winpid 2>/dev/null)"
			taskkill //F //PID "$winpid" > /dev/null 2>&1
			echo "$tag: HUNG after ${TIMEOUT}s (killed winpid $winpid)" >> "$RESULT"
			wait "$pid" 2>/dev/null
			return
		fi
		sleep 1
		waited=$((waited + 1))
	done
	wait "$pid"
	echo "$tag: exit $? (${waited}s)" >> "$RESULT"
}

cd "$FAKE/Build/BatchFiles"
for pass in 1 2
do
	run_tool "pass${pass}_headertool" "../../Bin/DevelopEngine/JGHeaderTool.exe"
	run_tool "pass${pass}_buildtool" "../../Bin/DevelopEngine/JGBuildTool.exe"
	cp "$FAKE/Source/Programs/JGBuildTool/jgengine.lua" "$WORK/pass${pass}_generated_jgengine.lua" 2>/dev/null
	cp "$FAKE/Temp/CodeGen/module_system_info.json" "$WORK/pass${pass}_module_system_info.json" 2>/dev/null
done

# 툴이 Step 4 에서 죽어 premake(Step 5)까지 못 가므로, Step 3 이 쓴 lua 를 premake 에 직접 넣어 본다 (수동 단계)
cp "$FAKE/Source/Programs/JGBuildTool/jgengine.lua" "$FAKE/jgengine.lua" 2>/dev/null
"../premake5.exe" vs2022 --file=../../jgengine.lua > "$WORK/manual_premake_stdout.txt" 2>&1
echo "manual premake: exit $?" >> "$RESULT"

ls -R "$FAKE/Temp" > "$WORK/fake_engine_temp_tree.txt" 2>&1
ls -R "$PROJ" > "$WORK/project_tree.txt" 2>&1
echo "done: $WORK" >> "$RESULT"
