#include <Windows.h>
#include <fstream>
#include "variable.h"

void clearRecordLoad() {
	std::ifstream in{ "clearRecord.txt" };

	for (int i = 0; i < 5; i++) {
		in >> stageCleared[i];
	}
}