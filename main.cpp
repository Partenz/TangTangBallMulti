#include <Windows.h>
#include <tchar.h>
#include <fstream>
#include <mmsystem.h>
#include "function.h"
#include "resource.h"
#include "variable.h"

#pragma comment (lib, "msimg32.lib")
#pragma comment(lib, "winmm.lib")

LPCTSTR lpszClass = L"Window Class Name";
LPCTSTR lpszWindowName = L"TangTangBall";

LRESULT CALLBACK WndProc(HWND hWnd, UINT iMessage, WPARAM wParam, LPARAM lParam);

// 윈도우 클래스 등록 함수 선언
ATOM RegisterGameWindow(HINSTANCE hinstance);
// 게임 초기화 함수 -  WndProc에서 CREATE 처리 호출
void CleanupGame(HWND hWnd);


int WINAPI WinMain(HINSTANCE hInstance, HINSTANCE hPrevInstance, LPSTR lpszCmdParam, int nCmdShow)
{
	HWND hWnd;
	MSG Message;
	
	g_hInst = hInstance;

	if (!RegisterGameWindow(hInstance))
	{
		return 1;
	}

	hWnd = CreateWindow(lpszClass, lpszWindowName, WS_OVERLAPPEDWINDOW, 0, 0, 1500, 840, NULL, (HMENU)NULL, hInstance, NULL);

	ShowWindow(hWnd, nCmdShow);
	UpdateWindow(hWnd);

	while (GetMessage(&Message, 0, 0, 0)) {
		TranslateMessage(&Message);
		DispatchMessage(&Message);
	}

	return Message.wParam;
}

ATOM RegisterGameWindow(HINSTANCE hInstance)
{
	WNDCLASSEX WndClass{};

	WndClass.cbSize = sizeof(WndClass);
	WndClass.style = CS_HREDRAW | CS_VREDRAW;
	WndClass.lpfnWndProc = (WNDPROC)WndProc;
	WndClass.cbClsExtra = 0;
	WndClass.cbWndExtra = 0;
	WndClass.hInstance = hInstance;
	WndClass.hIcon = LoadIcon(NULL, IDI_APPLICATION);
	WndClass.hCursor = LoadCursor(NULL, IDC_ARROW);
	WndClass.hbrBackground = (HBRUSH)GetStockObject(WHITE_BRUSH);
	WndClass.lpszMenuName = NULL;
	WndClass.lpszClassName = lpszClass;
	WndClass.hIconSm = LoadIcon(NULL, IDI_APPLICATION);
	
	// 등록 결과를 WinMain으로 전달
	return RegisterClassEx(&WndClass);
}


void CleanupGame(HWND hWnd)
{
	clearRecordSave();
	KillTimer(hWnd, 1);
	KillTimer(hWnd, 2);
	KillTimer(hWnd, 3);
}

LRESULT CALLBACK WndProc(HWND hWnd, UINT iMessage, WPARAM wParam, LPARAM lParam)
{
	PAINTSTRUCT ps;
	HDC hDC, mDC, uiDC, stageDC;
	HBITMAP hBitmap;
	HBRUSH hBrush, oldBrush;
	HFONT oldFont;
	


	switch (iMessage) {
	case WM_CREATE:
		InitializeGame(hWnd);
		break;

	case WM_PAINT:
		hDC = BeginPaint(hWnd, &ps);

		mDC = CreateCompatibleDC(hDC); //--- 메모리 DC 만들기
		hBitmap = CreateCompatibleBitmap(hDC, clientRect.right, clientRect.bottom); //--- 메모리 DC와 연결할 비트맵 만들기
		SelectObject(mDC, (HBITMAP)hBitmap);
		FillRect(mDC, &clientRect, (HBRUSH)GetStockObject(WHITE_BRUSH));

		uiDC = CreateCompatibleDC(hDC);
		stageDC = CreateCompatibleDC(hDC);

		if (gamestart) {
			GetClientRect(hWnd, &clientRect);
			// 격자 그리기
			DrawGrid(mDC, clientRect);

			// 스테이지 그리기
			DrawStage(mDC);

			//플레이어 원 그리기
			{
				COLORREF playerColor;
				if(doubleJumpMode)
					 playerColor= RGB(100, 30, 30);
				else
					playerColor = RGB(15, 255, 80);
				DrawPlayer(mDC, playerX, playerY, playerSize, playerColor);
			}

			if (stageClear) { // 스테이어 클리어 시
				playerMoveStart = false;
				DrawStageClear(mDC, uiDC, g_hCustomFont1);
			}

		}
		else if (not gamestart and stageManager) {
			DrawStageSelect(mDC, stageDC);
		}
		else if (not gamestart and stage == 0) {
			DrawGameStart(mDC, uiDC, g_hCustomFont, g_hCustomFont2);
		}
		
		BitBlt(hDC, 0, 0, clientRect.right, clientRect.bottom, mDC, 0, 0, SRCCOPY);

		DeleteDC(stageDC); // 생성한 스테이지 메모리 DC 삭제
		DeleteDC(uiDC); //--- 생성한 메모리 DC 삭제
		DeleteDC(mDC); //--- 생성한 메모리 DC 삭제
		DeleteObject(hBitmap);


		EndPaint(hWnd, &ps);
		break;

	case WM_CHAR:
		switch (wParam) {
		case 'r':
			if (gamestart) {
				playerMoveStart = false;
				doubleJumpMode = false;
				stageClear = false;
				mouseX = playerX = stageStartPoint.x;
				mouseY = playerY = stageStartPoint.y;
				mapDelete();
				mapLoad();
				playerJumpStartY = playerY + playerSize - 3;
				playerJumpHeight = playerJumpStartY - gridSpacing * 2;
			}
			break;

		case 'o':
			LMBHold = false;
			RMBHold = false;
			DialogBox(g_hInst, MAKEINTRESOURCE(IDD_DIALOG1), hWnd, (DLGPROC)mapTool);
			break;

		case VK_ESCAPE:
			gamestart = false;
			stageManager = false;
			LMBHold = false;
			RMBHold = false;
			SetBlockTypeSelect = false;
			playerPointSet = false;
			blockDeleteMode = false;
			stageClear = false;
			if (stage != 0)
				mapDelete();
			stage = 0;
			KillTimer(hWnd, 1);
			KillTimer(hWnd, 3);


			// 바탕화면 애니메이션 타이머 시작
			SetTimer(hWnd, 2, aniTimerSpeed, (TIMERPROC)menuAniTimer);
			break;
		}
	
		InvalidateRect(hWnd, NULL, false);
		break;

	case WM_LBUTTONDOWN:
		mouseX = LOWORD(lParam);
		mouseY = HIWORD(lParam);
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
		else if (not LMBHold and (GetAsyncKeyState(VK_RBUTTON) & 0x8000)) {
			playerMoveStart = true;
			LMBHold = true;
			RightMoving = true;
		}
		else if (not LMBHold) {
			playerMoveStart = true;
			LMBHold = true;
		}
		break;

	case WM_LBUTTONUP:
		if (not gamestart and stage == 0 and not stageManager) // 메인 메뉴
		{
			if (mouseX > 650 and mouseX < 846 and mouseY > 300 and mouseY < 382) // 시작 버튼
			{
				buttonNum[0] = 0;
				stageManager = true;
				KillTimer(hWnd, 2);
			}
			else if (mouseX > 650 and mouseX < 846 and mouseY > 450 and mouseY < 532) // 종료 버튼
			{
				clearRecordSave();
				buttonNum[1] = 0;
				KillTimer(hWnd, 1);
				KillTimer(hWnd, 2);
				KillTimer(hWnd, 3);
				PostQuitMessage(0);
				return 0;
				break;
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
				mouseX = playerX = stageStartPoint.x;
				mouseY = playerY = stageStartPoint.y;
				mapDelete();
				mapLoad();

				stageClear = false;
			}
			else if (PtInRect(&homeRT, pt)) { // 홈 버튼
				PlaySound(L"ButtonSound.wav", NULL, SND_ASYNC);
				gamestart = false;
				doubleJumpMode = false;
				stageManager = false;
				LMBHold = false;
				SetBlockTypeSelect = false;
				playerPointSet = false;
				blockDeleteMode = false;
				stageClear = false;
				if (stage != 0)
					mapDelete();
				stage = 0;
				KillTimer(hWnd, 1);
				KillTimer(hWnd, 3);

				// 바탕화면 애니메이션 타이머 시작
				SetTimer(hWnd, 2, aniTimerSpeed, (TIMERPROC)menuAniTimer);
				
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
				if(stage < 5)
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
					stageManager = false;
					gamestart = true;
					PlaySound(L"ButtonSound.wav", NULL, SND_ASYNC);
					stage = i + 1;

					playerX = stageStartPoint.x;
					playerY = stageStartPoint.y;

					mapLoad(); // 맵 불러오기

					SetTimer(hWnd, 1, moveTimerSpeed, (TIMERPROC)MoveTimer);
					SetTimer(hWnd, 3, 100, (TIMERPROC)checkStageClear);
				}
			}
		}
		else if (LMBHold) {
			LMBHold = false;
		}
		InvalidateRect(hWnd, NULL, false);
		break;

	case WM_RBUTTONUP:
		if (RMBHold) {
			RMBHold = false;
		}
		break;

	case WM_RBUTTONDOWN: // 블럭 설치하기 
		if (not RMBHold and (GetAsyncKeyState(VK_LBUTTON) & 0x8000) and not SetBlockTypeSelect and not playerPointSet and not blockDeleteMode) {
			playerMoveStart = true;
			RMBHold = true;
			RightMoving = false;
		}
		else if (not RMBHold and not SetBlockTypeSelect and not playerPointSet and not blockDeleteMode) {
			playerMoveStart = true;
			RMBHold = true;
		}
		else if (SetBlockTypeSelect) { // 설치 블럭 선택된 경우에만
			playerMoveStart = false;
			LMBHold = false;
			RMBHold = false;
			int x = LOWORD(lParam);
			int y = HIWORD(lParam);
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
			int x = LOWORD(lParam);
			int y = HIWORD(lParam);
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
			mpt.x = LOWORD(lParam);
			mpt.y = HIWORD(lParam);


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
		break;

	case WM_DESTROY:
		CleanupGame(hWnd);
		PostQuitMessage(0);
		return 0;
	}

	return (DefWindowProc(hWnd, iMessage, wParam, lParam));
}



void CALLBACK MoveTimer(HWND hWnd, UINT uMsg, UINT_PTR idEvent, DWORD dwTime)
{
	UpdatePlayerMovement();
	InvalidateRect(hWnd, NULL, false);
}

void CALLBACK menuAniTimer(HWND hWnd, UINT uMsg, UINT_PTR idEvent, DWORD dwTime)
{
	menuAniNum += 1;
	if (menuAniNum >= 9)
	{
		menuAniNum = 0;
	}
	InvalidateRect(hWnd, NULL, false);
}

void CALLBACK checkStageClear(HWND hWnd, UINT uMsg, UINT_PTR idEvent, DWORD dwTime)
{
	if (starCnt == 0 and playerMoveStart) StageClear();
	InvalidateRect(hWnd, NULL, false);
}

BOOL CALLBACK mapTool(HWND hDlg, UINT iMessage, WPARAM wParam, LPARAM lParam)
{
	static int radio{BlockType_Basic}; // 무슨 버튼 선택했는지 저장

	switch (iMessage) {
	case WM_INITDIALOG:
		SetBlockType = BlockType_Basic;
		break;

	case WM_COMMAND:
		switch (LOWORD(wParam)) {
		case IDC_RADIO1:
			radio = BlockType_Basic;
			break;
		case IDC_RADIO2:
			radio = BlockType_Thorn;
			break;
		case IDC_RADIO3:
			radio = BlockType_Once;
			break;
		case IDC_RADIO4:
			radio = BlockType_Jump;
			break;
		case IDC_RADIO5:
			radio = BlockType_Star;
			break;
		case IDC_RADIO6:
			radio = BlockType_DoubleJump;
			break;

		case IDC_RADIO7:
			radio = BlockType_UpDown;
			break;

		case IDC_BUTTON1: // 맵 저장
			mapSave();
			SetBlockTypeSelect = false;
			blockDeleteMode = false;
			playerPointSet = false;
			EndDialog(hDlg, false);
			break;

		case IDC_BUTTON3: // 플레이어 위치 선택
			playerPointSet = true;
			blockDeleteMode = false;
			SetBlockTypeSelect = false;
			EndDialog(hDlg, false);
			break;

		case IDOK: // 선택한 블럭
			SetBlockType = radio;
			blockDeleteMode = false;
			SetBlockTypeSelect = true;
			playerPointSet = false;
			EndDialog(hDlg, false);
			break;

		case IDCANCEL:
			SetBlockTypeSelect = false;
			blockDeleteMode = true;
			playerPointSet = false;
			EndDialog(hDlg, false);
			break;
		}
		break;

	case WM_CLOSE:
		EndDialog(hDlg, false);
		break;
	}
	return false;
}
