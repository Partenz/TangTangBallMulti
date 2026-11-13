#pragma once
#include "EventQueue.h"
#include "ServerVariable.h"

DWORD RunTcpSendLoop(const TcpSendConnection& connection, std::queue<NetEvent>& queue, HANDLE gate, HANDLE ready);
