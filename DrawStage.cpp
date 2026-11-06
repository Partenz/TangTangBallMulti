#include <Windows.h>
#include "variable.h"


void DrawStage(HDC hDC)
{
	HDC mDC = CreateCompatibleDC(hDC);

	Block* p = firstBlockPointer;
	for (int i = 0; i < BlockCnt; i++) {
		if (p != nullptr) {
			SelectObject(mDC, (HBITMAP)BlockBitmap[p->BlockType]); // 블럭 비트맵 선택

			if (p->BlockType == BlockType_Thorn)
				StretchBlt(hDC, p->rt.left + 1, p->rt.top + 1 - gridSpacing / 2, gridSpacing - 1, gridSpacing - 1, mDC, 0, 0, 100, 100, SRCCOPY); // 비트맵 그리기
			else if (p->BlockType == BlockType_Star)
				StretchBlt(hDC, p->rt.left - 2, p->rt.top, p->rt.right - p->rt.left + 5, p->rt.bottom - p->rt.top + 5, mDC, 0, 0, 32, 32, SRCCOPY); // 비트맵 그리기
			else if (p->BlockType == BlockType_LightningOnBlock
				or p->BlockType == BlockType_LightningOffBlock) {
				// 전기 BMP의 바깥 4픽셀 여백을 제외해 테두리를 격자에 맞춥니다.
				StretchBlt(hDC, p->rt.left, p->rt.top,
					p->rt.right - p->rt.left, p->rt.bottom - p->rt.top,
					mDC, 4, 4, 92, 92, SRCCOPY);
			}
			else
				StretchBlt(hDC, p->rt.left, p->rt.top, p->rt.right - p->rt.left, p->rt.bottom - p->rt.top, mDC, 0, 0, 100, 100, SRCCOPY); // 비트맵 그리기
			p = p->next;
		}
	}

	DeleteDC(mDC);
}
