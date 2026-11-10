#pragma once
#include "EventQueue.h"

// 클라이언트 TCP 송신 큐. 송신 스레드도 이 큐와 핸들을 사용합니다.
extern std::queue<NetEvent> clientTcpSendQueue;
extern HANDLE hClientTcpSendGate;
extern HANDLE hClientTcpSendReady;

// 스레드 시작 전 InitSyncEvents(hClientTcpSendGate, hClientTcpSendReady) 호출.
// body는 헤더 없는 본문입니다. 인코딩 후 큐 등록 성공 여부를 반환합니다.
// 실제 send는 이후 ClientTcpSendThreadMain에서 수행합니다.
bool QueueTcpPacket(BYTE packetType, const std::vector<BYTE>& body);
