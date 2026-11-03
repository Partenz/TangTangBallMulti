#include <Windows.h>
#include "variable.h"

void DrawGrid(HDC mDC, RECT clientRect)
{
	for (int y = 0; y <= clientRect.bottom; y += gridSpacing) {
		MoveToEx(mDC, 0, y, NULL);
		LineTo(mDC, clientRect.right, y);
	}
	for (int x = 0; x <= clientRect.right; x += gridSpacing) {
		MoveToEx(mDC, x, 0, NULL);
		LineTo(mDC, x, clientRect.bottom);
	}
}