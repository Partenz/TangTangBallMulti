#pragma once
#include <WinSock2.h>
#include <cstdint>
#include <queue>
#include <vector>

// 프로그램 내부 사건 번호이며 네트워크 패킷 번호와는 별개입니다.
enum NetEventKind : BYTE {
    NetEventPacket = 0,
    NetEventConnected = 1,
    NetEventDisconnected = 2
};

struct NetEvent {
    BYTE eventKind = NetEventPacket;
    BYTE playerId = 0; // 해당 없음은 0
    BYTE packetType = 0;
    // 수신/게임 큐: DecodePacket으로 분리한 본문.
    // 송신 큐: EncodePacket으로 만든 헤더 포함 전체 패킷.
    std::vector<BYTE> packetData;
};

struct TcpSendConnection {
    SOCKET socket = INVALID_SOCKET;
    HANDLE stopEvent = nullptr;       // 프로그램 전체 종료
    HANDLE disconnectEvent = nullptr; // 해당 연결 종료
};

// 서버 접속 및 UDP 등록 상태
extern SOCKET clientTcpSocket;
extern std::uint8_t clientPlayerId;
extern std::uint64_t clientUdpKey;
extern SOCKET clientUdpSocket;
extern sockaddr_in serverUdpAddress;
extern HANDLE hStopEvent;
extern HANDLE hUdpRegisteredEvent;

// TCP 송신 큐
extern std::queue<NetEvent> clientTcpSendQueue;
extern HANDLE hClientTcpSendGate;
extern HANDLE hClientTcpSendReady;

// UDP 송신 큐
extern std::queue<NetEvent> clientUdpSendQueue;
extern HANDLE hClientUdpSendGate;
extern HANDLE hClientUdpSendReady;
extern std::uint32_t clientUdpSendSequence;

// 클라이언트 수신 큐
extern std::queue<NetEvent> clientReceiveQueue;
extern HANDLE hClientReceiveGate;
extern HANDLE hClientReceiveReady;
