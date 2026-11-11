#pragma once
#include "TcpSendThread.h"

struct ServerTcpSendContext : TcpSendConnection {
    std::queue<NetEvent> sendQueue;
    HANDLE sendQueueGate = nullptr;
    HANDLE sendQueueReady = nullptr;
};

DWORD WINAPI ServerTcpSendThreadMain(LPVOID parameter);
