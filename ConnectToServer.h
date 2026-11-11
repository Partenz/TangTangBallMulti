#pragma once

#include <WinSock2.h>
#include <cstdint>

extern SOCKET clientTcpSocket;
extern std::uint8_t clientPlayerId;
extern std::uint64_t clientUdpKey;

// 서버에 연결하는 함수
bool ConnectToServer(const char* serverIp, std::uint16_t tcpPort);
