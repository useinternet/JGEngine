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
};
