#pragma once
#include "EventQueue.h"

DWORD RunTcpReceiveLoop(SOCKET socket, HANDLE stopEvent, HANDLE disconnectEvent);