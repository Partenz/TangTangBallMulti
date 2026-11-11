#pragma once
#include "EventQueue.h"

struct TcpSendConnection {
    SOCKET socket = INVALID_SOCKET;
    HANDLE stopEvent = nullptr;       // 프로그램 전체 종료
    HANDLE disconnectEvent = nullptr; // 해당 연결 종료
};

DWORD RunTcpSendLoop(const TcpSendConnection& connection, std::queue<NetEvent>& queue, HANDLE gate, HANDLE ready);
