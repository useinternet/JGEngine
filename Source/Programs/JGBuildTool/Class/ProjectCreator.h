#pragma once
#include "Core.h"

// 게임 프로젝트 만들기. CreateGameProject.bat → JGBuildTool -newproject=<ProjectDir> [-name=<Name>]
// Build/Templates/GameProject 를 복사하면서 파일 내용 · 파일 이름 · 폴더 이름의 토큰을 바꾼다.
//   {PROJECT_NAME}        MyGame
//   {PROJECT_NAME_UPPER}  MYGAME            define · API 매크로 (HeaderTool 이 모듈명을 대문자로 써서 _MYGAME 을 찾는다)
//   {ENGINE_ROOT}         C:/JG/JGEngine    .jgproject
//   {ENGINE_ROOT_WIN}     C:\JG\JGEngine    배치 파일
class PProjectCreator
{
public:
	// 이름을 비우면 폴더 이름을 쓴다.
	static bool Create(const PString& projectDirectory, const PString& projectName);

	// 프로젝트 경로에 ASCII 가 아닌 문자가 있으면 Critical 로그를 남기고 false.
	// 툴은 경로를 ANSI(fs::path::string)로 다루고 premake 는 UTF-8 로 읽어서, 한글 경로면 솔루션 생성에서 premake 가 실패한다.
	// 배치 1(-newproject=) 과 배치 2(-project=) 가 처음에 부른다. 공백은 된다(2026-09-30 공백 경로에서 생성 · 빌드 · 실행 확인).
	static bool IsSupportedProjectPath(const PString& projectDirectory);
};
