#pragma once
#include <WinSock2.h>
#include <cstdint>
#include <queue>
#include <vector>
#include <unordered_map>

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
extern HANDLE hQueueGate;
extern HANDLE hQueueReady;
extern HANDLE hConnectionGate;
extern HANDLE hStopEvent;
extern HANDLE hDisconnectEvent;
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

// 클라이언트 TCP 송신 루프에서 사용하는 연결 정보
extern TcpSendConnection clientTcpConnection;


struct PlayerInfo {
    uint8_t playerId;
    uint8_t color;
    bool ready;
};

struct ObjectInfo {
    uint32_t objectId;
    uint8_t blockType;
    int left, top, right, bottom;
    bool used;
};

struct PlayerMotion {
    uint8_t playerId;
    float x, y;
    float vx, vy;
    bool falling;
};

struct PlayerState {
    PlayerInfo info;
    uint8_t score;
    bool alive;
    BYTE assignedBlockType;
    bool placed;
    bool doubleJumpReady;
    bool mapReady;
    PlayerMotion motion;
    ULONGLONG lastMoveTime;
    int16_t previewGridX, previewGridY;
    bool previewVisible;
};

struct GameState {
    BYTE phase;
    uint16_t roundCnt;
    std::unordered_map<uint8_t, PlayerState> players;
    std::vector<ObjectInfo> objects;
    uint32_t nextObjectId;
    uint16_t mapWidth, mapHeight;
    float spawnX, spawnY;
    bool electricityActive;
    uint8_t roundWinnerId;
};

extern GameState clientGameState;