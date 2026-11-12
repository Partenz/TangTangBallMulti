#include "ServerVariable.h"

GameState gameState{};

SOCKET serverUdpSocket = INVALID_SOCKET;
HANDLE hStopEvent = nullptr;

std::queue<NetEvent> serverUdpSendQueue;
HANDLE hServerUdpSendGate = nullptr;
HANDLE hServerUdpSendReady = nullptr;

std::queue<NetEvent> gameRequestQueue;
HANDLE hGameRequestGate = nullptr;
HANDLE hGameRequestReady = nullptr;

std::unordered_map<std::uint8_t, std::uint64_t> issuedUdpKeys;
std::unordered_map<std::uint8_t, sockaddr_in> registeredUdpAddresses;