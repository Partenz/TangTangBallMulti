#include "function.h"


void CleanupGame(HWND hWnd)
{
	clearRecordSave();
	KillTimer(hWnd, 1);
	KillTimer(hWnd, 2);
	KillTimer(hWnd, 3);
}