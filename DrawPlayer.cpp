#include <Windows.h>

void DrawPlayer(HDC mDC, double playerX, double playerY, int playerSize, COLORREF playerColor)
{
	HBRUSH hBrush = CreateSolidBrush(playerColor);

	HBRUSH oldBrush = (HBRUSH)SelectObject(mDC, hBrush);
	Ellipse(mDC, playerX - playerSize, playerY - playerSize, playerX + playerSize, playerY + playerSize);
	SelectObject(mDC, oldBrush);
	DeleteObject(hBrush);
}
