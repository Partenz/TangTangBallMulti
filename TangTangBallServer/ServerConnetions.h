#pragma once
#include <WinSock2.h>
#include <cstdint>
#include <unordered_map>

enum class ConnectionState {
    Active,         // 통신 중
    Stopping,       // 종료 요청 처리 중
    ReadyToCollect  // 종료 요청 처리 완료, 스레드 종료 확인 가능
};

struct TcpConnection {
    SOCKET socket = INVALID_SOCKET;
    HANDLE sendThread = nullptr;
    HANDLE recvThread = nullptr;
    HANDLE disconnectEvent = nullptr;
    HANDLE sendQueueGate = nullptr;
    HANDLE sendQueueReady = nullptr;
    ConnectionState state = ConnectionState::Active;
};

extern std::unordered_map<std::uint8_t, TcpConnection> connections;
extern HANDLE hConnectionGate;

// 두 목록은 hConnectionGate로 보호한다.
// TODO: HandleNewConnection()에서 입장 승인 시 발급한 키를 저장한다.
extern std::unordered_map<std::uint8_t, std::uint64_t> issuedUdpKeys;
extern std::unordered_map<std::uint8_t, sockaddr_in> registeredUdpAddresses;

// 종료 요청을 기록하고 이벤트 설정
void RequestDisconnect(std::uint8_t playerId);
// 스레드가 끝났는지 확인, 이벤트 핸들을 닫고 제거
void CollectFinishedConnections();