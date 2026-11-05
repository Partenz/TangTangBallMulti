#include <Windows.h>
#include <fstream>
#include <string>
#include "variable.h"

void mapLoad()
{	
	const std::string fileName = "stage" + std::to_string(stage) + ".txt";
	
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