#pragma once
#include "EventQueue.h"

DWORD RunTcpReceiveLoop(SOCKET socket, HANDLE disconnectEvent, BYTE playerId);