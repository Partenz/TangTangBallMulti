#include "ServerGameThreadMain.h"
#include "EventQueue.h"
#include "HandleMoveInput.h"
#include "ServerConnections.h"
#include "HandleDoubleJump.h"
#include "UpdatePhysics.h"
#include "ServerCollision.h"
#include "ServerVariable.h"

extern GameState gameState;

// 서버 프로젝트에서 제공하는 핸들러용 플랜의 선언
void HandleReady(uint8_t playerId, bool ready) {}                                 // 준하 15일
void HandleMapReady(uint8_t playerId) {}                                          // 준하 16일
void HandlePlace(uint8_t playerId, int16_t gridX, int16_t gridY) {}               // 준홍 15일
//void HandleDoubleJump(uint8_t playerId, uint16_t roundCnt, uint8_t direction) {}  // 상준 13일 -완료
void HandlePreview(uint8_t playerId, uint16_t roundCnt, uint32_t sequence, int16_t gridX, int16_t gridY, bool visible) {} // 상준 17일
void HandleDisconnect(uint8_t playerId) {}                                        // 준홍 18일
void BeginRound() {}                                                              // 준홍 15일
void BeginRace() {}                                                               // 준홍 17일
void ReturnToLobby() {}                                                           // 준홍 18일
//void UpdatePhysics(float deltaSeconds) {}                                         // 상준 13일 -완료
//void CheckCollisions() {}                                                         // 준하 12일 -완료
void BroadcastPlayerStates() {}                                                   // 상준 15일

namespace {
    constexpr BYTE placingPhase = 2;
    constexpr BYTE racingPhase = 3;
    constexpr BYTE roundResultPhase = 4;
    constexpr BYTE gameResultPhase = 5;

    constexpr DWORD physicsIntervalMs = 15;
    //추진계획서에 상태 전송 주기와 결과 화면 표시 시간이 정해져 있지 않아서
    // 임의로 정해둔 값
    constexpr DWORD broadcastIntervalMs = 50;
    constexpr DWORD roundResultDurationMs = 2000;
    constexpr DWORD gameResultDurationMs = 5000;

    template<typename UInt>
    UInt ReadUnsigned(const std::vector<BYTE>& body, std::size_t offset)
    {
        UInt value = 0;
        for (std::size_t i = 0; i < sizeof(UInt); ++i)
            value = static_cast<UInt>((value << 8) | body[offset + i]);
        return value;
    }

    void DispatchGameRequest(const NetEvent& event)
    {
        const uint8_t playerId = event.playerId;
        if (playerId == 0)
            return;

        switch (event.eventKind) {
        case NetEventConnected:
            HandleNewConnection(playerId);
            return;
        case NetEventDisconnected:
            HandleDisconnect(playerId);
            return;
        case NetEventPacket:
            break;
        default:
            return;
        }

        const auto& body = event.packetData;
        switch (event.packetType) {
            // COLOR_REQUEST is assigned to the separate lobby implementation in the plan. // 준하 15일
        case 5: // READY_REQUEST
            if (body.size() == 1 && body[0] <= 1)
                HandleReady(playerId, body[0] != 0);
            break;
        case 7: // LEAVE_REQUEST
            if (body.empty())
                RequestDisconnect(playerId); // 접속 종료는 게임 요청 큐를 통해 처리한다// 준하 9일- 완료
            break;
        case 11: // MAP_READY
            if (body.empty())
                HandleMapReady(playerId);
            break;
        case 22: // PLACE_REQUEST
            if (body.size() == 4) {
                HandlePlace(playerId,
                    static_cast<int16_t>(ReadUnsigned<uint16_t>(body, 0)),
                    static_cast<int16_t>(ReadUnsigned<uint16_t>(body, 2)));
                if (gameState.phase == placingPhase)
                    BeginRace();
            }
            break;
        case 31: // DOUBLE_JUMP_REQUEST
            if (body.size() == 3)
                HandleDoubleJump(playerId, ReadUnsigned<uint16_t>(body, 0), body[2]);
            break;
        case 110: // MOVE_INPUT
            if (body.size() == 7)
                HandleMoveInput(playerId, ReadUnsigned<uint16_t>(body, 0),
                    ReadUnsigned<uint32_t>(body, 2), body[6]);
            break;
        case 120: // PREVIEW_STATE
            if (body.size() == 11 && body[10] <= 1)
                HandlePreview(playerId, ReadUnsigned<uint16_t>(body, 0),
                    ReadUnsigned<uint32_t>(body, 2),
                    static_cast<int16_t>(ReadUnsigned<uint16_t>(body, 6)),
                    static_cast<int16_t>(ReadUnsigned<uint16_t>(body, 8)),
                    body[10] != 0);
            break;
        }
    }
}

DWORD WINAPI ServerGameThreadMain(LPVOID unused)
{
    const HANDLE waitEvents[] = { hStopEvent, hGameRequestReady };
    std::vector<NetEvent> events;
    BYTE scheduledPhase = static_cast<BYTE>(-1);
    uint16_t scheduledRoundCnt = 0;
    ULONGLONG nextPhysics = 0;
    ULONGLONG nextBroadcast = 0;
    ULONGLONG resultDeadline = 0;

    const auto updateSchedule = [&](ULONGLONG now) {
        if (gameState.phase == scheduledPhase && gameState.roundCnt == scheduledRoundCnt)
            return;

        scheduledPhase = gameState.phase;
        scheduledRoundCnt = gameState.roundCnt;
        nextPhysics = now + physicsIntervalMs;
        nextBroadcast = now + broadcastIntervalMs;
        resultDeadline = now + (scheduledPhase == roundResultPhase ? roundResultDurationMs : gameResultDurationMs);
        };

    while (true) {
        const ULONGLONG now = GetTickCount64();
        updateSchedule(now);

        DWORD waitMs = INFINITE;
        ULONGLONG deadline = 0;
        if (scheduledPhase == racingPhase)
            deadline = nextPhysics < nextBroadcast ? nextPhysics : nextBroadcast;
        else if (scheduledPhase == roundResultPhase || scheduledPhase == gameResultPhase)
            deadline = resultDeadline;

        if (deadline != 0)
            waitMs = deadline > now ? static_cast<DWORD>(deadline - now) : 0;

        const DWORD waitResult = WaitForMultipleObjects(2, waitEvents, FALSE, waitMs);
        if (waitResult == WAIT_OBJECT_0)
            return 0;
        if (waitResult == WAIT_FAILED)
            return GetLastError();

        if (waitResult == WAIT_OBJECT_0 + 1) {
            events.clear();
            if (!DrainEventQueue(gameRequestQueue, events, hGameRequestGate, hGameRequestReady))
                return GetLastError();

            for (const NetEvent& event : events) {
                const DWORD stopResult = WaitForSingleObject(hStopEvent, 0);
                if (stopResult == WAIT_OBJECT_0)
                    return 0;
                if (stopResult == WAIT_FAILED)
                    return GetLastError();
                DispatchGameRequest(event);
                updateSchedule(GetTickCount64());
            }
        }

        const ULONGLONG tickTime = GetTickCount64();
        switch (gameState.phase) {
        case racingPhase:
            if (tickTime >= nextPhysics) {
                nextPhysics += physicsIntervalMs;
                UpdatePhysics(physicsIntervalMs / 1000.0f);
                CheckCollisions();
            }
            if (gameState.phase == racingPhase && tickTime >= nextBroadcast) {
                nextBroadcast = tickTime + broadcastIntervalMs;
                BroadcastPlayerStates();
            }
            break;
        case roundResultPhase:
            if (tickTime >= resultDeadline)
                BeginRound();
            break;
        case gameResultPhase:
            if (tickTime >= resultDeadline)
                ReturnToLobby();
            break;
        }
    }
}
