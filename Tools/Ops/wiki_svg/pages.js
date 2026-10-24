// 위키 문서 목록 — 그림 원본 폴더 specs/<key>/ 와 위키 문서를 잇는다.
// 그림은 문서와 같은 폴더의 그림/ 에 <file>.svg 로 쓰고, 문서에서는 ![설명](그림/<file>.svg) 로 넣는다.
// <file> 이름은 "문서키_번호_내용"(빈칸 없이), 예: Core_06_모듈연결, JGEngine전체구조_00_모듈계층
'use strict';
const W = 'C:/obsidian/ProjectJG/02. 위키/JGEngine';
const M = W + '/JGEngine Module';
const G = M + '/02. GameFrameWork';
module.exports = {
  JGEngine전체구조: W + '/JGEngine 전체 구조.md',
  JGEngine프로젝트구성도: W + '/JGEngine 프로젝트 구성도.md',
  JGEngine팀구성및운영: W + '/JGEngine 팀 구성 및 운영.md',
  Core: M + '/00. Core.md',
  Graphics: M + '/01. Graphics.md',
  GameFrameWork: G + '/GameFrameWork 개요.md',
  GameFrameWork_World: G + '/01. 월드와 액터.md',
  GameFrameWork_GameMaster: G + '/02. 규칙 커널(GameMaster).md',
  GameFrameWork_Network: G + '/03. 네트워크.md',
  GameFrameWork_GameUI: G + '/00. Game UI.md',
  GameFrameWork_DataTable: G + '/04. 데이터 테이블.md',
  GameFrameWork_Asset: G + '/05. 에셋.md',
  GameFrameWork_Audio: G + '/06. 오디오.md',
  GameFrameWork_SelfTest: G + '/07. 자체 시험.md',
  GameFrameWork_GameHost: G + '/08. GameHost 모듈.md',
  GUI: M + '/03. GUI.md',
  JGEditor: M + '/04. JGEditor.md',
  Programs: M + '/05. Programs.md'
};
