#include "ClientVariable.h"

SOCKET clientTcpSocket = INVALID_SOCKET;
std::uint8_t clientPlayerId = 0;
std::uint64_t clientUdpKey = 0;
SOCKET clientUdpSocket{};
sockaddr_in serverUdpAddress{};
HANDLE hQueueGate = nullptr;
HANDLE hQueueReady = nullptr;
HANDLE hConnectionGate = nullptr;
HANDLE hStopEvent = nullptr;
HANDLE hDisconnectEvent = nullptr;
HANDLE hUdpRegisteredEvent = nullptr;

std::queue<NetEvent> clientTcpSendQueue;
HANDLE hClientTcpSendGate = nullptr;
HANDLE hClientTcpSendReady = nullptr;

std::queue<NetEvent> clientUdpSendQueue;
HANDLE hClientUdpSendGate = nullptr;
HANDLE hClientUdpSendReady = nullptr;
std::uint32_t clientUdpSendSequence = 0;

std::queue<NetEvent> clientReceiveQueue;
HANDLE hClientReceiveGate = nullptr;
HANDLE hClientReceiveReady = nullptr;

TcpSendConnection clientTcpConnection;