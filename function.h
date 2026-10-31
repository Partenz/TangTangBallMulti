#pragma once
#include "variable.h"

// 사용자 함수 선언 공간
// 플레이어 움직이는 타이머
void CALLBACK MoveTimer(HWND hWnd, UINT uMsg, UINT_PTR idEvent, DWORD dwTime);

// 메뉴 바탕화면 애니메이션 타이머
void CALLBACK menuAniTimer(HWND hWnd, UINT uMsg, UINT_PTR idEvent, DWORD dwTime);

// 스테이지 클리어 확인 타이머
void CALLBACK checkStageClear(HWND hWnd, UINT uMsg, UINT_PTR idEvent, DWORD dwTime);

// 맵툴 대화상자
BOOL CALLBACK mapTool(HWND hDlg, UINT iMessage, WPARAM wParam, LPARAM lParam);

// 스테이지 그리기
void DrawStage(HDC); 

// 맵 저장
void mapSave();

// 맵 불러오기
void mapLoad();

// 클리어 기록 저장
void clearRecordSave();

// 클리어 기록 불러오기
void clearRecordLoad();

// 맵 삭제하기
void blockDelete(Block*);
void mapDelete();