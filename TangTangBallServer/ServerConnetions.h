#pragma once
#include <cstdint>

// 종료 요청을 기록하고 이벤트 설정
void RequestDisconnect(std::uint8_t playerId);
// 스레드가 끝났는지 확인, 이벤트 핸들을 닫고 제거
void CollectFinishedConnections();