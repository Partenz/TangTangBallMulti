#include <Windows.h>
#include "variable.h"

void DrawStageClear(HDC mDC, HDC uiDC, HFONT hFont)
{
	HFONT oldFont;

	HBRUSH hBrush = CreateSolidBrush(RGB(187, 201, 202));
	HBRUSH oldBrush = (HBRUSH)SelectObject(mDC, hBrush);
	RoundRect(mDC, stageClearBox.left, stageClearBox.top, stageClearBox.right, stageClearBox.bottom, 10, 10);
	SelectObject(mDC, oldBrush);
	DeleteObject(hBrush);

	// 1. 폰트 적용 (WM_CREATE에서 생성한 폰트를 사용)
	if (hFont) {
		oldFont = (HFONT)SelectObject(mDC, hFont); // DC에 폰트 선택

		// 2. 텍스트 색상 및 배경 모드 설정
		SetTextColor(mDC, RGB(255, 255, 0)); // 텍스트 색상을 노란색으로 설정
		SetBkColor(mDC, RGB(187, 201, 202));      // 텍스트 배경을 배경색과 동일하게 설정

		// 3. 텍스트 출력
		TextOut(mDC, 400, 150, L"STAGE CLEAR", lstrlen(L"STAGE CLEAR"));

		// 다시 원래의 폰트 (g_hCustomFont)를 사용하기 전 상태로 되돌립니다.
		SelectObject(mDC, oldFont); // 처음 선택했던 폰트(g_hCustomFont)를 사용하기 전 상태로 되돌림
	}

	SelectObject(uiDC, retry); // 다시하기 버튼
	BitBlt(mDC, retryRT.left, retryRT.top, retryRT.right, retryRT.bottom, uiDC, 0, 0, SRCCOPY);
	SelectObject(uiDC, home); // 홈 버튼
	BitBlt(mDC, homeRT.left, homeRT.top, homeRT.right, homeRT.bottom, uiDC, 0, 0, SRCCOPY);
	if (stage < 5) {
		SelectObject(uiDC, nextStage); // 다음 스테이지 버튼
		BitBlt(mDC, nextStageRT.left, nextStageRT.top, nextStageRT.right, nextStageRT.bottom, uiDC, 0, 0, SRCCOPY);
	}

}