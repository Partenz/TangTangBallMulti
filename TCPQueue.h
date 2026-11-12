#pragma once
#include "EventQueue.h"

// 스레드 시작 전 InitSyncEvents(hClientTcpSendGate, hClientTcpSendReady) 호출.
// body는 헤더 없는 본문입니다. 인코딩 후 큐 등록 성공 여부를 반환합니다.
// 실제 send는 이후 ClientTcpSendThreadMain에서 수행합니다.
bool QueueTcpPacket(BYTE packetType, const std::vector<BYTE>& body);
