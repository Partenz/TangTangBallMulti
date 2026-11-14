#include <Windows.h>
#include <tchar.h>
#include "function.h"
#include "resource.h"

#pragma comment(lib, "msimg32.lib")

bool LoadMenuBitmaps(HINSTANCE instance, HBITMAP (&frames)[9], HBITMAP (&buttons)[2])
{
    const int frameIds[] = {
        IDB_BITMAP2, IDB_BITMAP3, IDB_BITMAP4, IDB_BITMAP5, IDB_BITMAP6,
        IDB_BITMAP7, IDB_BITMAP8, IDB_BITMAP9, IDB_BITMAP10
    };
    bool loaded = true;
    for (UINT i = 0; i < ARRAYSIZE(frameIds); ++i) {
        frames[i] = LoadBitmapW(instance, MAKEINTRESOURCEW(frameIds[i]));
        if (frames[i] == nullptr)
            loaded = false;
    }

    buttons[0] = LoadBitmapW(instance, MAKEINTRESOURCEW(IDB_BITMAP16));
    buttons[1] = LoadBitmapW(instance, MAKEINTRESOURCEW(IDB_BITMAP18));
    return loaded && buttons[0] != nullptr && buttons[1] != nullptr;
}

void DrawGameStart(HDC target, HDC bitmapDC, const RECT& bounds, HBITMAP frame)
{
    const HGDIOBJ previous = SelectObject(bitmapDC, frame);
    StretchBlt(target, bounds.left, bounds.top, bounds.right - bounds.left,
        bounds.bottom - bounds.top, bitmapDC, 0, 0, 1500, 560, SRCCOPY);
    SelectObject(bitmapDC, previous);
}

void DrawMenuButtonBitmap(HDC target, HDC bitmapDC, const RECT& bounds, HBITMAP bitmap)
{
    const HGDIOBJ previous = SelectObject(bitmapDC, bitmap);
    TransparentBlt(target, bounds.left, bounds.top, bounds.right - bounds.left,
        bounds.bottom - bounds.top, bitmapDC, 0, 0, 96, 32, RGB(255, 255, 255));
    SelectObject(bitmapDC, previous);
}

void DrawGameStart(HDC mDC, HDC uiDC, HFONT g_hCustomFont, HFONT g_hCustomFont2)
{
	HFONT oldFont;

	DrawGameStart(mDC, uiDC, clientRect, menuAni[menuAniNum]);
	DrawMenuButtonBitmap(mDC, uiDC, RECT{ 650, 300, 846, 382 }, buttonUI[buttonNum[0]]);
	DrawMenuButtonBitmap(mDC, uiDC, RECT{ 650, 450, 846, 532 }, buttonUI[buttonNum[1]]);

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
		TextOut(mDC, 400, 100, text2, lstrlen(text2));

		// 다시 원래의 폰트 (g_hCustomFont)를 사용하기 전 상태로 되돌립니다.
		SelectObject(mDC, oldFont); // 처음 선택했던 폰트(g_hCustomFont)를 사용하기 전 상태로 되돌림
	}
	// 1. 폰트 적용 (WM_CREATE에서 생성한 폰트를 사용)
	if (g_hCustomFont2) {
		oldFont = (HFONT)SelectObject(mDC, g_hCustomFont2); // DC에 폰트 선택

		SetTextColor(mDC, RGB(0, 0, 0)); // 초록색 계열 (15,255,80 = #0FFF50)
		SetBkColor(mDC, RGB(140, 238, 238));      // 텍스트 배경을 투명하게 설정
		LPCTSTR text2 = _T("Tang Tang Ball");
		TextOut(mDC, 365, 100, text2, lstrlen(text2));

		// 다시 원래의 폰트 (g_hCustomFont)를 사용하기 전 상태로 되돌립니다.
		SelectObject(mDC, oldFont); // 처음 선택했던 폰트(g_hCustomFont)를 사용하기 전 상태로 되돌림
	}
	else {
		// g_hCustomFont가 NULL인 경우 (폰트 생성 실패 등)
		LPCTSTR errorText = _T("폰트를 로드할 수 없습니다. 기본 폰트로 표시됩니다.");
		TextOut(mDC, 50, 50, errorText, lstrlen(errorText));
	}
}
