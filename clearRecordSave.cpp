#include <Windows.h>
#include <fstream>
#include "variable.h"

void clearRecordSave() {
	std::ofstream out{ "clearRecord.txt" };

	for (int i = 0; i < 5; i++) {
		out << stageCleared[i] << " ";
	}
}