#pragma once

#include "ClientVariable.h"

// 서버에 연결하는 함수
bool ConnectToServer(const char* serverIp, std::uint16_t tcpPort);
