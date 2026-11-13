#pragma once
#include <WinSock2.h>
#include <cstdint>
#include <vector>
#include <queue>
#include <unordered_map>
#include <optional>

// 프로그램 내부 사건 번호이며 네트워크 패킷 번호와는 별개입니다.
enum NetEventKind : BYTE {
    NetEventPacket = 0,
    NetEventConnected = 1,
    NetEventDisconnected = 2
};


// 참가자 번호, 색상, 준비 상태
struct PlayerInfo {
    std::uint8_t playerId = 0;
    std::uint8_t color = 0;
    bool ready = false;
};

// 맵 오브젝트의 번호, 종류, 범위, 사용 여부
struct ObjectInfo {
    uint32_t objectId;
    uint8_t blockType;
    int left, top, right, bottom;
    bool used;
};

// 참가자의 위치, 속도, 낙하 상태
struct PlayerMotion {
    uint8_t playerId;
    float x, y;
    float vx, vy;
    bool flling;
};

// 참가자별 게임 상태와 최신 이동 입력
struct PlayerState {
    PlayerInfo info;
    uint8_t score;
    bool alive;
    BYTE assignedBlockType;
    bool placed;
    bool doubleJumpReady;
    bool mapReady;
    PlayerMotion motion;
    ULONGLONG lastmoveTime; // 마지막 유효 이동 입력을 처리한 시각(ms)
    int16_t previewGridX;
    int16_t previewGridY;
    bool previewVisible;
    std::optional<std::uint32_t> lastMoveSequence; // 마지막으로 처리한 이동 입력의 순서 번호
    std::uint8_t moveDirection = 0; // 0 정지, 1 왼쪽, 2 오른쪽
};

// 서버가 관리하는 현재 라운드, 참가자, 맵 상태
struct GameState {
    std::uint8_t phase = 0; // 로비 단계
    uint16_t roundCnt;
    std::unordered_map<std::uint8_t, PlayerState> players;
    std::vector<ObjectInfo> objects;
    uint32_t nextObjectId;
    uint16_t mapWidth, mapHeight;
    float spawnX, spawnY;
    bool electricityActive;
    uint8_t rountWinnerId;
};

// 게임 요청 큐와 송신 큐에서 전달하는 이벤트
struct NetEvent {
    BYTE eventKind = NetEventPacket;
    BYTE playerId = 0; // 해당 없음은 0
    BYTE packetType = 0;
    // 수신/게임 큐: DecodePacket으로 분리한 본문.
    // 송신 큐: EncodePacket으로 만든 헤더 포함 전체 패킷.
    std::vector<BYTE> packetData;
};

// 서버 UDP 소켓과 전체 종료 이벤트
extern SOCKET serverUdpSocket;
extern HANDLE hStopEvent;

// 서버 UDP 송신 큐와 큐 접근·데이터 도착 이벤트
extern std::queue<NetEvent> serverUdpSendQueue;
extern HANDLE hServerUdpSendGate;
extern HANDLE hServerUdpSendReady;

// 게임 요청 큐와 큐 접근·데이터 도착 이벤트
extern std::queue<NetEvent> gameRequestQueue;
extern HANDLE hGameRequestGate;
extern HANDLE hGameRequestReady;

// TCP 입장 승인 시, JOIN_RESPONSE를 보내기 전에 발급한 UDP 키를 저장한다.
extern std::unordered_map<std::uint8_t, std::uint64_t> issuedUdpKeys;

// UDP_REGISTER가 확인되면 여기에 송신 주소를 저장한다.
extern std::unordered_map<std::uint8_t, sockaddr_in> registeredUdpAddresses;

// TCP 연결 목록과 UDP 키·주소 목록에 대한 접근을 보호하는 이벤트
extern HANDLE hConnectionGate;

// TCP 송신 루프가 사용하는 소켓과 종료 이벤트
struct TcpSendConnection {
    SOCKET socket = INVALID_SOCKET;
    HANDLE stopEvent = nullptr;       // 서버 전체 종료를 알리는 이벤트
    HANDLE disconnectEvent = nullptr; // 이 클라이언트 연결의 종료를 알리는 이벤트
};

// 서버 TCP 송신 스레드
struct ServerTcpSendContext : TcpSendConnection {
    std::queue<NetEvent> sendQueue;
    HANDLE sendQueueGate = nullptr;
    HANDLE sendQueueReady = nullptr;
};

extern std::unordered_map<BYTE, ServerTcpSendContext*> serverTcpConnections;

extern HANDLE hQueueGate;
extern HANDLE hQueueReady;
extern HANDLE hDisconnectEvent;