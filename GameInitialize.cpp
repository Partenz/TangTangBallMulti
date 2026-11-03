#include <Windows.h>
#include <tchar.h>
#include "function.h"
#include "variable.h"
#include "resource.h"

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