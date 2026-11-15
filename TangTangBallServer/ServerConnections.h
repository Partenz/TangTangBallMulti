#pragma once

#include <memory>
#include "ServerTcpSendThreadMain.h"
#include "ServerVariable.h"

enum class ConnectionState {
    Active,         // 통신 중
    Stopping,       // 종료 요청 처리 중
    ReadyToCollect  // 종료 요청 처리 완료, 스레드 종료 확인 가능
};

struct TcpConnection {
    std::uint8_t playerId = 0;
    SOCKET socket = INVALID_SOCKET;
    HANDLE sendThread = nullptr;
    HANDLE recvThread = nullptr;
    HANDLE disconnectEvent = nullptr;
	std::shared_ptr<ServerTcpSendContext> sendContext = nullptr;
    ConnectionState state = ConnectionState::Active;
};

extern std::unordered_map<std::uint8_t, TcpConnection> connections;

// 소켓과 연결별 핸들을 닫는다. 시작한 스레드는 먼저 종료해야 한다.
void CloseTcpConnection(const TcpConnection& connection);

// 종료 요청을 기록하고 이벤트 설정
void RequestDisconnect(std::uint8_t playerId);
// 스레드가 끝났는지 확인, 이벤트 핸들을 닫고 제거
void CollectFinishedConnections();

// 게임 입장 처리
void HandleNewConnection(std::uint8_t playerId);
void HandleDisconnect(std::uint8_t playerId);

// TCP 송신 큐 패킷
bool QueueServerTcpPacket(std::uint8_t playerId, BYTE packetType,
    const std::vector<BYTE>& body);