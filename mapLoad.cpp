#include <Windows.h>
#include <fstream>
#include "variable.h"

void mapLoad()
{	
	std::string fileName{};

	if (stage == 1) {
		fileName = "stage1.txt";
	}
	else if (stage == 2) {
		fileName = "stage2.txt";
	}
	else if (stage == 3) {
		fileName = "stage3.txt";
	}
	else if (stage == 4) {
		fileName = "stage4.txt";
	}
	else if (stage == 5) {
		fileName = "stage5.txt";
	}

	std::ifstream in{ fileName };

	in >> BlockCnt >> starCnt >> stageStartPoint.x >> stageStartPoint.y;

	Block* prev = nullptr;
	for (int i = 0; i < BlockCnt; i++) {
		Block* p = new Block;
		if (i == 0)
			firstBlockPointer = p;
		in >> p->BlockType >> p->rt.left >> p->rt.top >> p->rt.right >> p->rt.bottom;

		if (prev != nullptr) {
			p->prev = prev;
			prev->next = p;
			p->next = firstBlockPointer;
			firstBlockPointer->prev = p;
		}
		else {
			p->next = p;
			p->prev = p;
		}
		prev = p;
	}
	playerX = stageStartPoint.x;
	playerY = stageStartPoint.y;
}