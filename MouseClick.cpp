#include <Windows.h>
#include "variable.h"
#include "function.h"

// 마우스 좌클릭 다운
void MLeftClickDown(HWND hWnd, int mouseX, int mouseY)
{
	if (not gamestart and stage == 0)
	{
		if (mouseX > 650 and mouseX < 846 and mouseY > 300 and mouseY < 382)
		{
			PlaySound(L"ButtonSound.wav", NULL, SND_ASYNC);
			buttonNum[0] = 1;
		}
		else if (mouseX > 650 and mouseX < 846 and mouseY > 450 and mouseY < 532)
		{
			PlaySound(L"ButtonSound.wav", NULL, SND_ASYNC);
			buttonNum[1] = 1;
		}
	}
	else if (not LMBHold) {
		playerMoveStart = true;
		LMBHold = true;
	}
}
// 마우스 좌클릭 업
void MLeftClickUp(HWND hWnd, int mouseX, int mouseY)
{
	if (not gamestart and stage == 0 and not stageManager) // 메인 메뉴
	{
		if (mouseX > 650 and mouseX < 846 and mouseY > 300 and mouseY < 382) // 시작 버튼
		{
			buttonNum[0] = 0;
			OpenStageSelect(hWnd);
		}
		else if (mouseX > 650 and mouseX < 846 and mouseY > 450 and mouseY < 532) // 종료 버튼
		{
			clearRecordSave();
			buttonNum[1] = 0;
			KillTimer(hWnd, 1);
			KillTimer(hWnd, 2);
			KillTimer(hWnd, 3);
			PostQuitMessage(0);
			return;
		}

	}
	else if (stageClear) {
		POINT pt{ mouseX, mouseY };
		if (PtInRect(&retryRT, pt)) { // 다시하기 버튼
			PlaySound(L"ButtonSound.wav", NULL, SND_ASYNC);
			playerMoveStart = false;
			doubleJumpMode = false;
			stageManager = false;
			LMBHold = false;
			SetBlockTypeSelect = false;
			playerPointSet = false;
			blockDeleteMode = false;
			playerX = stageStartPoint.x;
			playerY = stageStartPoint.y;
			mapDelete();
			mapLoad();

			stageClear = false;
		}
		else if (PtInRect(&homeRT, pt)) { // 홈 버튼
			PlaySound(L"ButtonSound.wav", NULL, SND_ASYNC);
			ReturnToMenu(hWnd);
		}
		else if (PtInRect(&nextStageRT, pt) and stage < 5) { // 다음 스테이지 버튼
			PlaySound(L"ButtonSound.wav", NULL, SND_ASYNC);
			stageManager = false;
			doubleJumpMode = false;
			LMBHold = false;
			SetBlockTypeSelect = false;
			playerPointSet = false;
			blockDeleteMode = false;
			mapDelete();
			if (stage < 5)
				stage++;
			mapLoad();
			playerX = stageStartPoint.x;
			playerY = stageStartPoint.y;
			SetTimer(hWnd, 1, moveTimerSpeed, (TIMERPROC)MoveTimer);

			stageClear = false;
		}
	}
	else if (stageManager and not gamestart) {

		POINT pt{ mouseX, mouseY };

		for (int i = 0; i < 5; i++) {
			if (PtInRect(&stageBitRect[i], pt)) {
				PlaySound(L"ButtonSound.wav", NULL, SND_ASYNC);

				StartStage(hWnd, i + 1);
				break;
			}
		}
	}
	else if (LMBHold) {
		LMBHold = false;
	}
	InvalidateRect(hWnd, NULL, false);
}
// 마우스 우클릭 다운
void MRightClickDown(HWND hWnd, int mouseX, int mouseY)
{
	if (not RMBHold and not SetBlockTypeSelect and not playerPointSet and not blockDeleteMode) {
		playerMoveStart = true;
		RMBHold = true;
	}
	else if (SetBlockTypeSelect) { // 설치 블럭 선택된 경우에만
		playerMoveStart = false;
		LMBHold = false;
		RMBHold = false;
		int x = mouseX;
		int y = mouseY;
		x -= x % gridSpacing;
		y -= y % gridSpacing;
		Block* p = firstBlockPointer;
		bool alreadySet = false; // 중복 검사에 걸리면 설치 안 하게
		for (int i = 0; i < BlockCnt; i++) { // 이미 설치된 블럭 있는지 검사
			if (p != nullptr) {
				if (x == p->rt.left and y == p->rt.top) {
					alreadySet = true;
					break;
				}
				p = p->next;
			}
		}
		if (not alreadySet) {
			Block* newP = new Block;
			p = firstBlockPointer;

			if (p != nullptr) { // 2개 이상의 블럭 생성일 때
				while (p->next != nullptr and p->next != firstBlockPointer)
					p = p->next;
				p->next = newP;
			}
			else // 첫 블럭 생성일 때
				p = firstBlockPointer = newP;
			newP->prev = p;
			newP->next = firstBlockPointer;
			firstBlockPointer->prev = newP;
			newP->BlockType = SetBlockType;
			newP->rt.left = x;
			if (SetBlockType == BlockType_Thorn)
				newP->rt.top = y + gridSpacing / 2;
			else
				newP->rt.top = y;
			newP->rt.right = x + gridSpacing;
			newP->rt.bottom = y + gridSpacing;
			if (SetBlockType == BlockType_Star) {
				starCnt++;
				newP->rt.left = x + gridSpacing / 4;
				newP->rt.top = y + gridSpacing / 4;
				newP->rt.right = x + gridSpacing - gridSpacing / 4;
				newP->rt.bottom = y + gridSpacing - gridSpacing / 4;
			}
			BlockCnt++;
			InvalidateRect(hWnd, NULL, false);
		}
	}
	else if (playerPointSet) {
		playerMoveStart = false;
		int x = static_cast<int>(mouseX);
		int y = static_cast<int>(mouseY);
		x -= x % gridSpacing;
		y -= y % gridSpacing;
		stageStartPoint.x = playerX = x + gridSpacing / 2;
		stageStartPoint.y = playerY = y + gridSpacing / 2;
		playerDropping = true; // 위치 설정하면 일단 무조건 낙하
		playerPointSet = false;
		InvalidateRect(hWnd, NULL, false);
	}
	else if (blockDeleteMode) {
		playerMoveStart = false;
		POINT mpt;
		mpt.x = static_cast<LONG>(mouseX);
		mpt.y = static_cast<LONG>(mouseY);


		Block* p = firstBlockPointer;
		for (int i = 0; i < BlockCnt; i++) {
			if (PtInRect(&p->rt, mpt)) {
				if (p->BlockType == BlockType_Star)
					starCnt--;
				blockDelete(p);
				break;
			}

			p = p->next;
		}

		InvalidateRect(hWnd, NULL, false);
	}
}
// 마우스 우클릭 업
void MRightClickUp(HWND hWnd, int mouseX, int mouseY)
{
	if (RMBHold) {
		RMBHold = false;
	}
}
