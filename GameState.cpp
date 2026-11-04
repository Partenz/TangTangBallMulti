#include "function.h"
#include "variable.h"

// 메인 메뉴 -> 스테이지 선택
void OpenStageSelect(HWND hWnd)
{
	gamestart = false;
	stageManager = true;

	KillTimer(hWnd, 2);
}

// 스테이지 선택 -> 시작
void StartStage(HWND hWnd, int stageNumber)
{
	stageManager = false;
	gamestart = true;

	stage = stageNumber;

	playerX = stageStartPoint.x;
	playerY = stageStartPoint.y;

	mapLoad();

	SetTimer(hWnd, 1, moveTimerSpeed, (TIMERPROC)MoveTimer);
	SetTimer(hWnd, 3, 100, (TIMERPROC)checkStageClear);
}

// 스테이지 -> 메인 메뉴
void ReturnToMenu(HWND hWnd)
{
	gamestart = false;
	stageManager = false;

	LMBHold = false;
	RMBHold = false;
	doubleJumpMode = false;

	SetBlockTypeSelect = false;
	playerPointSet = false;
	blockDeleteMode = false;
	stageClear = false;

	if (stage != 0)
	{
		mapDelete();
	}

	stage = 0;

	KillTimer(hWnd, 1);
	KillTimer(hWnd, 3);

	// 바탕화면 애니메이션 타이머 시작
	SetTimer(hWnd, 2, aniTimerSpeed, (TIMERPROC)menuAniTimer);
}