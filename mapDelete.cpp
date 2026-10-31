#include <Windows.h>
#include "variable.h"

// 멥 초기화
void mapDelete()
{
	Block* p = firstBlockPointer->next;
	while (p->next != firstBlockPointer) {
		Block* temp = p;
		
		p = p->next;

		delete temp;
	}

	delete firstBlockPointer;
	firstBlockPointer = nullptr;
}

// 블럭 하나만 삭제
void blockDelete(Block* p) {
	if (p == firstBlockPointer) {
		p->next->prev = p->prev;
		p->prev->next = p->next;

		firstBlockPointer = p->next;
	}
	else {
		p->next->prev = p->prev;
		p->prev->next = p->next;
	}

	delete p;
	--BlockCnt;
}