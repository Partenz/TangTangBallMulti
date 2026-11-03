#include <Windows.h>
#include <tchar.h>
#include "variable.h"

void DrawGameStart(HDC mDC, HDC uiDC, HFONT g_hCustomFont, HFONT g_hCustomFont2)
{
	HFONT oldFont;

	SelectObject(uiDC, menuAni[menuAniNum]); //--- 4) 비트맵 선택하기
	//BitBlt(mDC, 0, 0, clientRect.right, clientRect.bottom, uiDC, 0, 0, SRCCOPY);
	StretchBlt(mDC, 0, 0, clientRect.right, clientRect.bottom, uiDC, 0, 0, 1500, 560, SRCCOPY);

	//ui button
	SelectObject(uiDC, buttonUI[buttonNum[0]]); //--- 4) 비트맵 선택하기
	TransparentBlt(mDC, 650, 300, 196, 82, uiDC, 0, 0, 96, 32, RGB(255, 255, 255));

	SelectObject(uiDC, buttonUI[buttonNum[1]]); //--- 4) 비트맵 선택하기
	TransparentBlt(mDC, 650, 450, 196, 82, uiDC, 0, 0, 96, 32, RGB(255, 255, 255));

	// 1. 폰트 적용 (WM_CREATE에서 생성한 폰트를 사용)
	if (g_hCustomFont) {
		oldFont = (HFONT)SelectObject(mDC, g_hCustomFont); // DC에 폰트 선택

		// 2. 텍스트 색상 및 배경 모드 설정
		SetTextColor(mDC, RGB(0, 0, 255)); // 텍스트 색상을 파란색으로 설정
		SetBkColor(mDC, RGB(199, 199, 199));      // 텍스트 배경을 투명하게 설정

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