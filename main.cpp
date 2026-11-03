#include <Windows.h>
#include <tchar.h>
#include <fstream>
#include <mmsystem.h>
#include "function.h"
#include "resource.h"
#include "variable.h"

#pragma comment (lib, "msimg32.lib")
#pragma comment(lib, "winmm.lib")

HINSTANCE g_hInst;
HFONT g_hCustomFont = NULL;
HFONT g_hCustomFont1 = NULL;
HFONT g_hCustomFont2 = NULL;
LPCTSTR lpszClass = L"Window Class Name";
LPCTSTR lpszWindowName = L"TangTangBall";

LRESULT CALLBACK WndProc(HWND hWnd, UINT iMessage, WPARAM wParam, LPARAM lParam);

// 윈도우 클래스 등록 함수 선언
ATOM RegisterGameWindow(HINSTANCE hinstance);
// 게임 초기화 함수 -  WndProc에서 CREATE 처리 호출
void InitializeGame(HWND hWnd);
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

void InitializeGame(HWND hWnd)
{
	//폰트
	LOGFONT lf{}, lf1{}, lf2{};
	
	// 플레이어 히트박스 설정
	playerRT.left = playerX - playerSize;
	playerRT.top = playerY - playerSize;
	playerRT.right = playerX + playerSize;
	playerRT.bottom = playerY + playerSize;

	GetClientRect(hWnd, &clientRect);
	// 시작메뉴 바탕 애니메이션 비트맵
	menuAni[0] = LoadBitmap(g_hInst, MAKEINTRESOURCE(IDB_BITMAP2));
	menuAni[1] = LoadBitmap(g_hInst, MAKEINTRESOURCE(IDB_BITMAP3));
	menuAni[2] = LoadBitmap(g_hInst, MAKEINTRESOURCE(IDB_BITMAP4));
	menuAni[3] = LoadBitmap(g_hInst, MAKEINTRESOURCE(IDB_BITMAP5));
	menuAni[4] = LoadBitmap(g_hInst, MAKEINTRESOURCE(IDB_BITMAP6));
	menuAni[5] = LoadBitmap(g_hInst, MAKEINTRESOURCE(IDB_BITMAP7));
	menuAni[6] = LoadBitmap(g_hInst, MAKEINTRESOURCE(IDB_BITMAP8));
	menuAni[7] = LoadBitmap(g_hInst, MAKEINTRESOURCE(IDB_BITMAP9));
	menuAni[8] = LoadBitmap(g_hInst, MAKEINTRESOURCE(IDB_BITMAP10));
	// 기본 블럭 비트맵
	BlockBitmap[BlockType_Basic] = LoadBitmap(g_hInst, MAKEINTRESOURCE(IDB_BITMAP1));
	// 특수블럭
	BlockBitmap[BlockType_Jump] = LoadBitmap(g_hInst, MAKEINTRESOURCE(IDB_BITMAP11));
	BlockBitmap[BlockType_Once] = LoadBitmap(g_hInst, MAKEINTRESOURCE(IDB_BITMAP12));
	BlockBitmap[BlockType_Thorn] = LoadBitmap(g_hInst, MAKEINTRESOURCE(IDB_BITMAP13));
	BlockBitmap[BlockType_Star] = LoadBitmap(g_hInst, MAKEINTRESOURCE(IDB_BITMAP14));
	BlockBitmap[BlockType_DoubleJump] = LoadBitmap(g_hInst, MAKEINTRESOURCE(IDB_BITMAP15));
	BlockBitmap[BlockType_UpDown] = LoadBitmap(g_hInst, MAKEINTRESOURCE(IDB_BITMAP17));
	// 버튼 ui
	buttonUI[0] = LoadBitmap(g_hInst, MAKEINTRESOURCE(IDB_BITMAP16));
	buttonUI[1] = LoadBitmap(g_hInst, MAKEINTRESOURCE(IDB_BITMAP18));
	// 스테이지 비트맵
	stageBitmap[0] = LoadBitmap(g_hInst, MAKEINTRESOURCE(IDB_BITMAP19));
	stageBitmap[1] = LoadBitmap(g_hInst, MAKEINTRESOURCE(IDB_BITMAP20));
	stageBitmap[2] = LoadBitmap(g_hInst, MAKEINTRESOURCE(IDB_BITMAP21));
	stageBitmap[3] = LoadBitmap(g_hInst, MAKEINTRESOURCE(IDB_BITMAP22));
	stageBitmap[4] = LoadBitmap(g_hInst, MAKEINTRESOURCE(IDB_BITMAP23));

	// 스테이지 클리어 버튼 비트맵
	retry = LoadBitmap(g_hInst, MAKEINTRESOURCE(IDB_BITMAP26));
	home = LoadBitmap(g_hInst, MAKEINTRESOURCE(IDB_BITMAP24));
	nextStage = LoadBitmap(g_hInst, MAKEINTRESOURCE(IDB_BITMAP25));

	// 스테이지 클리어 버튼 영역
	retryRT = { 350,400,470,520 };
	homeRT = { 650, 390, 810, 530 };
	nextStageRT = { 990, 400, 1110, 520 };

	// 바탕화면 애니메이션 타이머 시작
	SetTimer(hWnd, 2, aniTimerSpeed, (TIMERPROC)menuAniTimer);

	// 클리어 기록 불러오기
	clearRecordLoad();

	// 폰트 생성
	{
		ZeroMemory(&lf, sizeof(LOGFONT)); // LOGFONT 구조체 초기화

		// 폰트 속성 설정
		lf.lfHeight = -30; // 폰트 높이 (논리 단위, 음수면 문자 셀 높이 지정)
		// 약 30pt 크기를 나타냅니다.
		lf.lfWeight = FW_BOLD; // 굵기: 굵게 (FW_NORMAL, FW_LIGHT, FW_BLACK 등)
		lf.lfCharSet = DEFAULT_CHARSET; // 문자 집합 (한글을 위해 HANGUL_CHARSET도 가능)
		lf.lfItalic = FALSE; // 기울임꼴 (TRUE/FALSE)
		lf.lfUnderline = FALSE; // 밑줄 (TRUE/FALSE)
		lf.lfStrikeOut = FALSE; // 취소선 (TRUE/FALSE)
		// 폰트 패밀리 (PitchAndFamily)는 보통 DEFAULT_PITCH | FF_DONTCARE 또는 FF_SWISS 등
		// lf.lfPitchAndFamily = DEFAULT_PITCH | FF_SWISS;

		// 폰트 이름 설정 (반드시 설치된 폰트여야 합니다)
		// _tcscpy_s는 유니코드/멀티바이트 문자열에 모두 안전하게 사용
		_tcscpy_s(lf.lfFaceName, LF_FACESIZE, _T("맑은 고딕"));
		// 또는 L"맑은 고딕" (와이드 문자열 리터럴)

		// 폰트 생성
		g_hCustomFont = CreateFontIndirect(&lf);

		if (g_hCustomFont == NULL) {
			// 폰트 생성 실패 시 에러 처리 (예: MessageBox)
			MessageBox(hWnd, _T("폰트 생성 실패!"), _T("오류"), MB_ICONERROR);
		}
	}
	// 폰트 생성
	{
		ZeroMemory(&lf1, sizeof(LOGFONT)); // LOGFONT 구조체 초기화

		// 폰트 속성 설정
		lf1.lfHeight = -100; // 폰트 높이 (논리 단위, 음수면 문자 셀 높이 지정)
		// 약 30pt 크기를 나타냅니다.
		lf1.lfWeight = FW_BOLD; // 굵기: 굵게 (FW_NORMAL, FW_LIGHT, FW_BLACK 등)
		lf1.lfCharSet = DEFAULT_CHARSET; // 문자 집합 (한글을 위해 HANGUL_CHARSET도 가능)
		lf1.lfItalic = FALSE; // 기울임꼴 (TRUE/FALSE)
		lf1.lfUnderline = FALSE; // 밑줄 (TRUE/FALSE)
		lf1.lfStrikeOut = FALSE; // 취소선 (TRUE/FALSE)
		// 폰트 패밀리 (PitchAndFamily)는 보통 DEFAULT_PITCH | FF_DONTCARE 또는 FF_SWISS 등
		// lf.lfPitchAndFamily = DEFAULT_PITCH | FF_SWISS;

		// 폰트 이름 설정 (반드시 설치된 폰트여야 합니다)
		// _tcscpy_s는 유니코드/멀티바이트 문자열에 모두 안전하게 사용
		_tcscpy_s(lf1.lfFaceName, LF_FACESIZE, _T("맑은 고딕"));
		// 또는 L"맑은 고딕" (와이드 문자열 리터럴)

		// 폰트 생성
		g_hCustomFont1 = CreateFontIndirect(&lf1);

		if (g_hCustomFont1 == NULL) {
			// 폰트 생성 실패 시 에러 처리 (예: MessageBox)
			MessageBox(hWnd, _T("폰트 생성 실패!"), _T("오류"), MB_ICONERROR);
		}
	}
	{
		ZeroMemory(&lf2, sizeof(LOGFONT)); // LOGFONT 구조체 초기화

		// 폰트 속성 설정
		lf2.lfHeight = -100; // 폰트 높이 (논리 단위, 음수면 문자 셀 높이 지정)
		// 약 30pt 크기를 나타냅니다.
		lf2.lfWeight = FW_BOLD; // 굵기: 굵게 (FW_NORMAL, FW_LIGHT, FW_BLACK 등)
		lf2.lfCharSet = DEFAULT_CHARSET; // 문자 집합 (한글을 위해 HANGUL_CHARSET도 가능)
		lf2.lfItalic = FALSE; // 기울임꼴 (TRUE/FALSE)
		lf2.lfUnderline = FALSE; // 밑줄 (TRUE/FALSE)
		lf2.lfStrikeOut = FALSE; // 취소선 (TRUE/FALSE)
		// 폰트 패밀리 (PitchAndFamily)는 보통 DEFAULT_PITCH | FF_DONTCARE 또는 FF_SWISS 등
		// lf.lfPitchAndFamily = DEFAULT_PITCH | FF_SWISS;

		// 폰트 이름 설정 (반드시 설치된 폰트여야 합니다)
		// _tcscpy_s는 유니코드/멀티바이트 문자열에 모두 안전하게 사용
		_tcscpy_s(lf2.lfFaceName, LF_FACESIZE, _T("맑은 고딕"));
		// 또는 L"맑은 고딕" (와이드 문자열 리터럴)

		// 폰트 생성
		g_hCustomFont2 = CreateFontIndirect(&lf2);

		if (g_hCustomFont2 == NULL) {
			// 폰트 생성 실패 시 에러 처리 (예: MessageBox)
			MessageBox(hWnd, _T("폰트 생성 실패!"), _T("오류"), MB_ICONERROR);
		}
	}

	stageClearBox.left = 240;
	stageClearBox.top = 100;
	stageClearBox.right = 1240;
	stageClearBox.bottom = 700;
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
			for (int y = 0; y <= clientRect.bottom; y += gridSpacing) {
				MoveToEx(mDC, 0, y, NULL);
				LineTo(mDC, clientRect.right, y);
			}
			for (int x = 0; x <= clientRect.right; x += gridSpacing) {
				MoveToEx(mDC, x, 0, NULL);
				LineTo(mDC, x, clientRect.bottom);
			}

			// 스테이지 그리기
			DrawStage(mDC);

			//플레이어 원 그리기
			{
				if(doubleJumpMode)
					hBrush = CreateSolidBrush(RGB(100, 30, 30));
				else
					hBrush = CreateSolidBrush(RGB(15, 255, 80));
				oldBrush = (HBRUSH)SelectObject(mDC, hBrush);
				Ellipse(mDC, playerX - playerSize, playerY - playerSize, playerX + playerSize, playerY + playerSize);
				SelectObject(mDC, oldBrush);
				DeleteObject(hBrush);
			}

			if (stageClear) { // 스테이어 클리어 시
				playerMoveStart = false;

				hBrush = CreateSolidBrush(RGB(187, 201, 202));
				oldBrush = (HBRUSH)SelectObject(mDC, hBrush);
				RoundRect(mDC, stageClearBox.left, stageClearBox.top, stageClearBox.right, stageClearBox.bottom, 10, 10);
				SelectObject(mDC, oldBrush);
				DeleteObject(hBrush);

				// 1. 폰트 적용 (WM_CREATE에서 생성한 폰트를 사용)
				if (g_hCustomFont1) {
					oldFont = (HFONT)SelectObject(mDC, g_hCustomFont1); // DC에 폰트 선택

					// 2. 텍스트 색상 및 배경 모드 설정
					SetTextColor(mDC, RGB(255, 255, 0)); // 텍스트 색상을 파란색으로 설정
					SetBkColor(mDC, RGB(187, 201, 202));      // 텍스트 배경을 투명하게 설정

					// 3. 텍스트 출력
					TextOut(mDC, 400, 150, L"STAGE CLEAR", lstrlen(L"STAGE CLEAR"));

					// 다시 원래의 폰트 (g_hCustomFont)를 사용하기 전 상태로 되돌립니다.
					SelectObject(mDC, oldFont); // 처음 선택했던 폰트(g_hCustomFont)를 사용하기 전 상태로 되돌림
				}

				SelectObject(uiDC, retry); // 다시하기 버튼
				BitBlt(mDC,retryRT.left, retryRT.top, retryRT.right, retryRT.bottom,uiDC,0,0,SRCCOPY);
				SelectObject(uiDC, home); // 홈 버튼
				BitBlt(mDC, homeRT.left, homeRT.top, homeRT.right, homeRT.bottom, uiDC, 0, 0, SRCCOPY);
				if (stage < 5) {
					SelectObject(uiDC, nextStage); // 다음 스테이지 버튼
					BitBlt(mDC, nextStageRT.left, nextStageRT.top, nextStageRT.right, nextStageRT.bottom, uiDC, 0, 0, SRCCOPY);
				}
			}

		}
		else if (not gamestart and stageManager) {
			hBrush = CreateSolidBrush(RGB(140, 238, 238));
			oldBrush = (HBRUSH) SelectObject(mDC, hBrush);
			FillRect(mDC, &clientRect, hBrush);
			SelectObject(mDC, oldBrush);
			DeleteObject(hBrush);

			for (int i = 0; i < 5; i++) {
				SelectObject(stageDC, stageBitmap[i]);
				stageBitRect[i].left = 15 + i * 300;
				stageBitRect[i].top = 150;
				stageBitRect[i].right = 265 + i * 300;
				stageBitRect[i].bottom = 650;
				BitBlt(mDC, stageBitRect[i].left, stageBitRect[i].top, stageBitRect[i].right, stageBitRect[i].bottom, stageDC, 0, 0, SRCCOPY);

				if (stageCleared[i]) {
					SetBkColor(mDC, RGB(140, 238, 238));
					TextOut(mDC, (stageBitRect[i].left + stageBitRect[i].right) / 2 - 40, stageBitRect[i].top - 50, L"Stage Cleared", _tcslen(L"Stage Cleared"));
				}
			}
		}
		else if (not gamestart and stage == 0) {
			SelectObject(uiDC, menuAni[menuAniNum]); //--- 4) 비트맵 선택하기
			//BitBlt(mDC, 0, 0, clientRect.right, clientRect.bottom, uiDC, 0, 0, SRCCOPY);
			StretchBlt(mDC, 0, 0, clientRect.right, clientRect.bottom, uiDC, 0, 0, 1500, 560, SRCCOPY);

			//ui button
			SelectObject(uiDC, buttonUI[buttonNum[0]]); //--- 4) 비트맵 선택하기
			TransparentBlt(mDC, 650, 300, 196, 82, uiDC, 0, 0, 96, 32, RGB(255,255,255));

			SelectObject(uiDC, buttonUI[buttonNum[1]]); //--- 4) 비트맵 선택하기
			TransparentBlt(mDC, 650, 450, 196, 82, uiDC, 0, 0, 96, 32, RGB(255, 255, 255));

			// 1. 폰트 적용 (WM_CREATE에서 생성한 폰트를 사용)
			if (g_hCustomFont) {
				oldFont = (HFONT)SelectObject(mDC, g_hCustomFont); // DC에 폰트 선택

				// 2. 텍스트 색상 및 배경 모드 설정
				SetTextColor(mDC, RGB(0, 0, 255)); // 텍스트 색상을 파란색으로 설정
				SetBkColor(mDC, RGB(199,199,199));      // 텍스트 배경을 투명하게 설정

				// 3. 텍스트 출력
				TextOut(mDC, 700, 320, L"START", lstrlen(L"START"));

				TextOut(mDC, 720, 470, L"EXIT", lstrlen(L"EXIT"));


				//텍스트 색상 변경 (예시)
				SetTextColor(mDC, RGB(0, 0, 0)); // 초록색 계열 (15,255,80 = #0FFF50)
				SetBkColor(mDC, RGB(140, 238, 238));      // 텍스트 배경을 투명하게 설정
				LPCTSTR text2 = _T("Tang Tang Ball");
				TextOut(mDC, 400, 100, text2, _tcslen(text2));

				// 다시 원래의 폰트 (g_hCustomFont)를 사용하기 전 상태로 되돌립니다.
				SelectObject(mDC, oldFont); // 처음 선택했던 폰트(g_hCustomFont)를 사용하기 전 상태로 되돌림
			}
			// 1. 폰트 적용 (WM_CREATE에서 생성한 폰트를 사용)
			if (g_hCustomFont2) {
				oldFont = (HFONT)SelectObject(mDC, g_hCustomFont2); // DC에 폰트 선택

				SetTextColor(mDC, RGB(0, 0, 0)); // 초록색 계열 (15,255,80 = #0FFF50)
				SetBkColor(mDC, RGB(140, 238, 238));      // 텍스트 배경을 투명하게 설정
				LPCTSTR text2 = _T("Tang Tang Ball");
				TextOut(mDC, 365, 100, text2, _tcslen(text2));

				// 다시 원래의 폰트 (g_hCustomFont)를 사용하기 전 상태로 되돌립니다.
				SelectObject(mDC, oldFont); // 처음 선택했던 폰트(g_hCustomFont)를 사용하기 전 상태로 되돌림
			}
			else {
				// g_hCustomFont가 NULL인 경우 (폰트 생성 실패 등)
				LPCTSTR errorText = _T("폰트를 로드할 수 없습니다. 기본 폰트로 표시됩니다.");
				TextOut(mDC, 50, 50, errorText, _tcslen(errorText));
			}
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

	Block* p = firstBlockPointer;
	RECT temp{};
	// x 이동
	for (int i = 0; i < BlockCnt; i++) { 
		if (playerRT.left >= p->rt.left and playerRT.left <= p->rt.right and playerY >= p->rt.top and playerY <= p->rt.bottom 
			and p->BlockType != BlockType_Star and p->BlockType != BlockType_DoubleJump) { // 플레이어 왼쪽 히트박스가 블럭내부로 들어옴
			if (p->BlockType != BlockType_UpDown) {
				playerX += playerSpeed * 0.7;
			}
			break;
		}
		else if (playerRT.right >= p->rt.left and playerRT.right <= p->rt.right and playerY >= p->rt.top and playerY <= p->rt.bottom 
			and p->BlockType != BlockType_Star and p->BlockType != BlockType_DoubleJump) { // 플레이어 오른쪽 히트박스가 블럭내부로 들어옴
			if (p->BlockType != BlockType_UpDown) {
				playerX -= playerSpeed * 0.7;
			}
			break;
		}
		else if (playerRT.left >= p->rt.left and playerRT.left <= p->rt.right and playerY >= p->rt.top and playerY <= p->rt.bottom and 
			(p->BlockType == BlockType_Star or p->BlockType == BlockType_DoubleJump)) { // 플레이어 왼쪽 히트박스가 블럭내부로 들어옴
			if (p->BlockType == BlockType_DoubleJump) { // 더블 점프 아이템 먹음
				if (RMBHold and not LMBHold) { // 우측 방향 이동
					playerX += playerSpeed * 0.7;
				}
				else if (LMBHold and not RMBHold) { // 좌측으로 이동 
					playerX -= playerSpeed * 0.7;
				}
				else if (LMBHold and RMBHold and doubleJumpMode) { // 더블 점프 모드라면 더블 점프, 아닌데 둘다 누르고 있으면 x 축 이동 없음
					playerDropping = false;
					PlaySound(L"DefaultJumpSound.wav", NULL, SND_ASYNC);
					playerJumpStartY = playerY + playerSize;
					playerJumpHeight = playerJumpStartY - gridSpacing * 3;
					doubleJumpMode = false;
					if (RightMoving) { // 우측 방향 더블 점프
						playerX += playerSpeed * 0.7;
					}
					else if (not RightMoving) { // 좌측 방향 더블 점프
						playerX -= playerSpeed * 0.7;
					}
				}
				Block* temp = p;
				p = p->prev;
				blockDelete(temp);
				doubleJumpMode = true;
			}
			else if (p->BlockType == BlockType_Star) { // 별 먹음
				PlaySound(L"StarSound.wav", NULL, SND_ASYNC);
				if (RMBHold and not LMBHold) { // 우측 방향 이동
					playerX += playerSpeed * 0.7;
				}
				else if (LMBHold and not RMBHold) { // 좌측으로 이동 
					playerX -= playerSpeed * 0.7;
				}
				else if (LMBHold and RMBHold and doubleJumpMode) { // 더블 점프 모드라면 더블 점프, 아닌데 둘다 누르고 있으면 x 축 이동 없음
					playerDropping = false;
					PlaySound(L"DefaultJumpSound.wav", NULL, SND_ASYNC);
					playerJumpStartY = playerY + playerSize;
					playerJumpHeight = playerJumpStartY - gridSpacing * 3;
					doubleJumpMode = false;
					if (RightMoving) { // 우측 방향 더블 점프
						playerX += playerSpeed * 0.7;
					}
					else if (not RightMoving) { // 좌측 방향 더블 점프
						playerX -= playerSpeed * 0.7;
					}
				}
				Block* temp = p;
				p = p->prev;
				blockDelete(temp);
				starCnt--;
			}
			break;
		}
		else if (playerRT.right >= p->rt.left and playerRT.right <= p->rt.right and playerY >= p->rt.top and playerY <= p->rt.bottom 
			and (p->BlockType == BlockType_Star or p->BlockType == BlockType_DoubleJump)) { // 플레이어 오른쪽 히트박스가 블럭내부로 들어옴
			if (p->BlockType == BlockType_DoubleJump) { // 더블 점프 아이템 먹음
				if (RMBHold and not LMBHold) { // 우측 방향 이동
					playerX += playerSpeed * 0.7;
				}
				else if (LMBHold and not RMBHold) { // 좌측으로 이동 
					playerX -= playerSpeed * 0.7;
				}
				else if (LMBHold and RMBHold and doubleJumpMode) { // 더블 점프 모드라면 더블 점프, 아닌데 둘다 누르고 있으면 x 축 이동 없음
					playerDropping = false;
					PlaySound(L"DefaultJumpSound.wav", NULL, SND_ASYNC);
					playerJumpStartY = playerY + playerSize;
					playerJumpHeight = playerJumpStartY - gridSpacing * 3;
					doubleJumpMode = false;
					if (RightMoving) { // 우측 방향 더블 점프
						playerX += playerSpeed * 0.7;
					}
					else if (not RightMoving) { // 좌측 방향 더블 점프
						playerX -= playerSpeed * 0.7;
					}
				}
				Block* temp = p;
				p = p->prev;
				blockDelete(temp);
				doubleJumpMode = true;
			}
			else if (p->BlockType == BlockType_Star) { // 별 먹음
				PlaySound(L"StarSound.wav", NULL, SND_ASYNC);
				if (RMBHold and not LMBHold) { // 우측 방향 이동
					playerX += playerSpeed * 0.7;
				}
				else if (LMBHold and not RMBHold) { // 좌측으로 이동 
					playerX -= playerSpeed * 0.7;
				}
				else if (LMBHold and RMBHold and doubleJumpMode) { // 더블 점프 모드라면 더블 점프, 아닌데 둘다 누르고 있으면 x 축 이동 없음
					playerDropping = false;
					PlaySound(L"DefaultJumpSound.wav", NULL, SND_ASYNC);
					playerJumpStartY = playerY + playerSize;
					playerJumpHeight = playerJumpStartY - gridSpacing * 3;
					doubleJumpMode = false;
					if (RightMoving) { // 우측 방향 더블 점프
						playerX += playerSpeed * 0.7;
					}
					else if (not RightMoving) { // 좌측 방향 더블 점프
						playerX -= playerSpeed * 0.7;
					}
				}
				Block* temp = p;
				p = p->prev;
				blockDelete(temp);
				starCnt--;
			}
			break;
		}
		else if (i == BlockCnt - 1) { // 모든 블럭과 충돌 안 함
			if (RMBHold and not LMBHold) { // 우측 방향 이동
				playerX += playerSpeed * 0.7;
			}
			else if (LMBHold and not RMBHold) { // 좌측으로 이동 
				playerX -= playerSpeed * 0.7;
			}
			else if (LMBHold and RMBHold and doubleJumpMode) { // 더블 점프 모드라면 더블 점프, 아닌데 둘다 누르고 있으면 x 축 이동 없음
				playerDropping = false;
				PlaySound(L"DefaultJumpSound.wav", NULL, SND_ASYNC);
				playerJumpStartY = playerY + playerSize;
				playerJumpHeight = playerJumpStartY - gridSpacing * 3;
				doubleJumpMode = false;
				if (RightMoving) { // 우측 방향 더블 점프
					playerX += playerSpeed * 0.7;
				}
				else if (not RightMoving) { // 좌측 방향 더블 점프
					playerX -= playerSpeed * 0.7;
				}
			}
		}
		if (p->next != firstBlockPointer)
			p = p->next;
	}
	// y 이동
	p = firstBlockPointer;
	if (playerDropping) {
		playerY += playerSpeed;
	}
	else if (not playerDropping) {
		playerY -= playerSpeed;
	}
	for (int i = 0; i < BlockCnt; i++) {
		if (playerX + playerSize / 4 >= p->rt.left and playerX - playerSize / 4 <= p->rt.right ) { // 블럭과 같은 열에 있음
			if (playerDropping and playerY + playerSize >= p->rt.top and playerY + playerSize <= p->rt.bottom and p->BlockType != BlockType_UpDown) { // 블럭과 충돌
				if (p->BlockType == BlockType_Basic) { // 일반 블럭과 충돌
					playerDropping = false;
					PlaySound(L"DefaultJumpSound.wav", NULL, SND_ASYNC);
					playerJumpStartY = playerY + playerSize - 3;
					playerJumpHeight = playerJumpStartY - gridSpacing * 2;
				}
				else if (p->BlockType == BlockType_Thorn) { // 가시 블럭과 충돌
					doubleJumpMode = false;
					PlaySound(L"DeadSound.wav", NULL, SND_ASYNC);
					playerX = stageStartPoint.x;
					playerY = stageStartPoint.y;
					mapDelete();
					mapLoad();
				}
				else if (p->BlockType == BlockType_Once) { // 일회용 블럭과 충돌
					playerDropping = false;
					PlaySound(L"OnceBlockSound.wav", NULL, SND_ASYNC);
					Block* temp = p;
					p = p->prev;
					blockDelete(temp);
					playerJumpStartY = playerY + playerSize - 3;
					playerJumpHeight = playerJumpStartY - gridSpacing * 2;
				}
				else if (p->BlockType == BlockType_Jump) { // 점프 블럭과 충돌
					playerDropping = false;
					PlaySound(L"JumpBlockSound.wav", NULL, SND_ASYNC);
					playerJumpStartY = playerY + playerSize - 3;
					playerJumpHeight = playerJumpStartY - gridSpacing * 3;
				}
		
				break;
			}
			else if (not playerDropping and playerY - playerSize >= playerJumpHeight) { // 점프
				if (playerY - playerSize <= p->rt.bottom and playerY - playerSize >= p->rt.top 
					and p->BlockType != BlockType_UpDown and p->BlockType != BlockType_DoubleJump and p->BlockType != BlockType_Star) { // 업다운 블럭이 아닌 블럭과 위로 충돌해 낙하 시작
					playerDropping = true;
				}
				else if (playerY - playerSize >= p->rt.top and playerY - playerSize <= p->rt.bottom and p->BlockType == BlockType_UpDown) { // 점프 최대 지점에서 업다운 블럭 만남
					playerDropping = false;
					playerJumpStartY = p->rt.top - 3;
					playerJumpHeight = playerJumpStartY - gridSpacing * 2;
				}
			}

			else if (not playerDropping and playerY - playerSize < playerJumpHeight) { // 점프 완료 후 다시 낙하
				playerDropping = true;
			}
		}
		else if (not playerDropping and playerY - playerSize < playerJumpHeight) { // 점프 완료 후 다시 낙하
			playerDropping = true;
		}

		if(p->next != firstBlockPointer)
			p = p->next;
	}

	if (playerY >= 840 and not SetBlockTypeSelect and not blockDeleteMode and not playerPointSet and not stageClear) { // 바닥으로 떨어짐
		playerMoveStart = false;
		doubleJumpMode = false;
		PlaySound(L"DeadSound.wav", NULL, SND_ASYNC);
		playerX = stageStartPoint.x;
		playerY = stageStartPoint.y;
		mapDelete();
		mapLoad();
		playerJumpStartY = playerY + playerSize - 3;
		playerJumpHeight = playerJumpStartY - gridSpacing * 2;
	}

	// 플레이어 히트박스 설정
	playerRT.left = playerX - playerSize;
	playerRT.top = playerY - playerSize;
	playerRT.right = playerX + playerSize;
	playerRT.bottom = playerY + playerSize;

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
	if (starCnt == 0 and playerMoveStart) { // 스테이지에 남은 별 없음
		PlaySound(L"StageClearSound.wav", NULL, SND_ASYNC);
		stageClear = true;
		stageCleared[stage - 1] = true;
		stageManager = false;
		doubleJumpMode = false;
		LMBHold = false;
		RMBHold = false;
		SetBlockTypeSelect = false;
		playerPointSet = false;
		blockDeleteMode = false;
	}

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
