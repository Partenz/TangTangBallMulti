#pragma once
#include <cstdint>

// UDP 순서 확인하는 함수
bool IsNewerSequence(uint32_t received, uint32_t last);
