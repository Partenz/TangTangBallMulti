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
	WndClass.style = CS_HREDRAW | CS_VREDRAW | CS_DBLCLKS;
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
				playerX = stageStartPoint.x;
				playerY = stageStartPoint.y;
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
			ReturnToMenu(hWnd);
			break;
		}
	
		InvalidateRect(hWnd, NULL, false);
		break;

	case WM_LBUTTONDOWN:
	case WM_LBUTTONDBLCLK:
		MLeftClickDown(hWnd, LOWORD(lParam), HIWORD(lParam));
		if (iMessage == WM_LBUTTONDBLCLK)
			TryUseDoubleJump();
		break;

	case WM_LBUTTONUP:
		MLeftClickUp(hWnd, LOWORD(lParam), HIWORD(lParam));
		break;

	case WM_RBUTTONUP:
		MRightClickUp(hWnd, LOWORD(lParam), HIWORD(lParam));
		break;

	case WM_RBUTTONDOWN: 
	case WM_RBUTTONDBLCLK:
		MRightClickDown(hWnd, LOWORD(lParam), HIWORD(lParam));
		if (iMessage == WM_RBUTTONDBLCLK)
			TryUseDoubleJump();
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
			SetMapEditMode(MapEditMode::None);
			EndDialog(hDlg, false);
			break;

		case IDC_BUTTON3: // 플레이어 위치 선택
			SetMapEditMode(MapEditMode::SetPlayerPoint);
			EndDialog(hDlg, false);
			break;

		case IDOK: // 선택한 블럭
			SetBlockType = radio;
			SetMapEditMode(MapEditMode::PlaceBlock);

			EndDialog(hDlg, false);
			break;

		case IDCANCEL:
			SetMapEditMode(MapEditMode::DeleteBlock);
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
