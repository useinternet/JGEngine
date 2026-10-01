#pragma once
#include "JGEditorDefine.h"
#include "Widget.h"
#include "DataTableEditor.generation.h"

class HDataTableEditorTab;

// 데이터 테이블 편집 창("Data Table Editor", 메뉴 Windows/Data Table Editor). GameFrameWorks Data/ 의 편집 모델(PDataTableDocument)을
// 스프레드시트 그리드(GUI PGUIGrid)로 보여 준다. 열린 테이블마다 탭 하나.
//   위: 테이블 열기 · 새 테이블 · 저장 · 다시 읽기 · 실행 취소 / 다시 실행 · 행 · 열 추가 · 필터
//   가운데: 그리드(키 열 · 머리 행 고정) + 오른쪽 열 속성(열 머리를 누르면)
//   아래: 문제 목록(누르면 그 칸으로) · 상태 줄
// 파일이 바깥에서 바뀌면(텍스트 편집기 · git · 다른 에이전트) 저장 안 된 변경이 없을 때 다시 읽고, 있으면 알림 줄을 띄운다.
JGCLASS()
class JGEDITOR_API JGDataTableEditor : public JGWidget
{
	JG_GENERATED_WIDGET_BODY

	friend class HDataTableEditorTab;
	friend class HDataTableEditorResolver;

private:
	HList<HSTLUniquePtr<HDataTableEditorTab>> _tabs;
	HDataTableEditorTab* _selectRequest = nullptr;   // 다음 프레임에 고를 탭
	HDataTableEditorTab* _closeRequest  = nullptr;   // 닫기 확인 중인 탭(저장 안 됨)
	HDataTableEditorTab* _saveConfirm   = nullptr;   // 저장 확인 중인 탭(모르는 열이 사라짐)
	bool                 _bSaveConfirmRequest = false;   // 다음 그리기에서 저장 확인 창을 연다(창 수준 ID)
	HDataTableEditorTab* _visibleTab    = nullptr;   // 이번 프레임에 그린 탭(도구 막대의 대상)

	PString _newTableContent;
	PString _newTablePath = "Data/NewTable";
	PString _newTableKeyColumn = "Id";
	PString _newTableError;
	bool    _bNewTableRequest = false;

	PString _statusText;
	bool    _bStatusError = false;
	int64   _lastFileCheckMs = 0;

public:
	// 탭 형식이 .cpp 에만 있어 생성 · 소멸자를 .cpp 에 둔다(HSTLUniquePtr 가 완전한 형식을 요구한다)
	JGDataTableEditor();
	virtual ~JGDataTableEditor();
	// dllexport 클래스는 복사 연산을 모두 만들어 탭 목록(HSTLUniquePtr)에서 막힌다 — 위젯은 복사하지 않는다
	JGDataTableEditor(const JGDataTableEditor&) = delete;
	JGDataTableEditor& operator=(const JGDataTableEditor&) = delete;

protected:
	virtual PString GetTitleName() const override;
	virtual void OnUpdate() override;
	virtual void OnGenerateGUI() override;

public:
	// 테이블을 탭으로 연다. 이미 열려 있으면 그 탭을 고른다. inTokenPath = /JGGame/… · /JGEngine/…
	bool OpenTable(const PString& inTokenPath);

private:
	void setStatus(const PString& inText, bool bError);
	void clearErrorStatus();
	HDataTableEditorTab* findTab(const PString& inTokenPath) const;
	void removeTab(HDataTableEditorTab* inTab);
	bool saveTab(HDataTableEditorTab& tab, bool bConfirmed);

	void drawToolbar(HDataTableEditorTab* activeTab);
	void drawTab(HDataTableEditorTab& tab);
	void drawModals();
	void drawNewTableModal();
	void drawCloseModal();
	void drawSaveConfirmModal();
};
