#include <Windows.h>
#include <tchar.h>
#include "variable.h"

void DrawStageSelect(HDC mDC, HDC stageDC)
{
	HBRUSH hBrush = CreateSolidBrush(RGB(140, 238, 238));
	HBRUSH oldBrush = (HBRUSH)SelectObject(mDC, hBrush);
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