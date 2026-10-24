// 위키 문서 목록 — 그림 원본 폴더 specs/<key>/ 와 위키 문서를 잇는다.
// 그림은 문서와 같은 폴더의 그림/ 에 <file>.svg 로 쓰고, 문서에서는 ![설명](그림/<file>.svg) 로 넣는다.
// <file> 이름은 "문서키_번호_내용"(빈칸 없이), 예: Core_06_모듈연결, JGEngine구조도_00_모듈의존
'use strict';
const W = 'C:/obsidian/ProjectJG/02. 위키/JGEngine';
const M = W + '/JGEngine Module';
module.exports = {
  Core: M + '/00.Core.md',
  JGEngine구조도: W + '/JGEngine 구조도.md',
  JGEngine빌드와실행: W + '/JGEngine 빌드와 실행.md',
  JGEngine팀운영: W + '/JGEngine 팀 운영.md',
  AI: M + '/AI.md',
  Asset: M + '/Asset.md',
  Audio: M + '/Audio.md',
  Devkit: M + '/Devkit.md',
  GUI: M + '/GUI.md',
  GameFrameWorks: M + '/GameFrameWorks.md',
  GameHost: M + '/GameHost.md',
  Graphics: M + '/01.Graphics.md',
  JGEditor: M + '/JGEditor와 도구 창.md',
  Server: M + '/네트워크(Server).md',
  엔진프로그램: M + '/엔진 프로그램.md'
};
