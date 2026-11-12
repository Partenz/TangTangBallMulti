#pragma once
#include <WinSock2.h>
#include <Windows.h>

bool CloseSyncEvents(HANDLE& hQueueGate, HANDLE& hQueueReady, HANDLE* hConnectionGate = nullptr, HANDLE* hStopEvent = nullptr, HANDLE* hDisconnectEvent = nullptr);
