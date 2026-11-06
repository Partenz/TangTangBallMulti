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

// 격자 그리기
void DrawGrid(HDC, RECT);

// 스테이지 그리기
void DrawStage(HDC); 

// 플레이어 그리기
void DrawPlayer(HDC, double, double, int, COLORREF);

// 클리어 화면 그리기
void DrawStageClear(HDC, HDC, HFONT);

// 스테이지 선택 화면 그리기
void DrawStageSelect(HDC, HDC);

// 게임 시작 화면 그리기
void DrawGameStart(HDC, HDC, HFONT, HFONT);

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

// 플레이어 움직임 업데이트
void UpdatePlayerMovement();

// 게임 중 더블클릭으로 보유한 더블 점프 아이템 사용
void TryUseDoubleJump();

// 게임 시작시 초기화
void InitializeGame(HWND hWnd);

// 스테이지 클리어
void StageClear();

// 마우스 좌클릭 다운
void MLeftClickDown(HWND, int, int);
// 마우스 좌클릭 업
void MLeftClickUp(HWND, int, int);
// 마우스 우클릭 다운
void MRightClickDown(HWND, int, int);
// 마우스 우클릭 업
void MRightClickUp(HWND, int, int);

// 스테이지 선택화면 열기
void OpenStageSelect(HWND hWnd);
// 스테이지 시작
void StartStage(HWND hWnd, int stageNumber);
// 메뉴로 돌아가기
void ReturnToMenu(HWND hWnd);

// 맵툴 모드 선택
void SetMapEditMode(MapEditMode mode);