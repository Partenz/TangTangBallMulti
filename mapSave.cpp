#include <Windows.h>
#include <fstream>
#include "variable.h"

void mapSave()
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
	std::ofstream out{ fileName };

	Block* p = firstBlockPointer;
	out << BlockCnt << "  " << starCnt << "  " << stageStartPoint.x << "   " << stageStartPoint.y << "   ";
	for (int i = 0; i < BlockCnt; i++) {
		out << p->BlockType << "   " << p->rt.left << "   " << p->rt.top << "  " << p->rt.right << "  " << p->rt.bottom << " ";
		if (p->next != nullptr)
			p = p->next;
	}
	SetBlockTypeSelect = false;
}