#ifndef _CRT_RAND_S
#define _CRT_RAND_S
#endif
#include <stdlib.h>

#include "ServerConnections.h"
#include "EnDePacket.h"
#include "EventQueue.h"

#include <new>
#include <vector>

GameState gameState{};

namespace {
    constexpr BYTE lobbyPhase = 0;
    constexpr BYTE joinResponseType = 1;
    constexpr BYTE lobbyStateType = 2;

    bool QueueServerTcpPacket(std::uint8_t playerId, BYTE packetType,
        const std::vector<BYTE>& body)
    {
        NetEvent event{};
        event.playerId = playerId;
        event.packetType = packetType;
        event.packetData = EncodePacket(packetType, body);

        if (event.packetData.empty() ||
            WaitForSingleObject(hConnectionGate, INFINITE) != WAIT_OBJECT_0) {
            return false;
        }

        const auto it = connections.find(playerId);
        bool queued = false;

        if (it != connections.end() &&
            it->second.state == ConnectionState::Active &&
            it->second.sendContext) {
            auto& context = *it->second.sendContext;
            queued = PushEventQueue(context.sendQueue, event,
                context.sendQueueGate,
                context.sendQueueReady);
        }

        SetEvent(hConnectionGate);
        return queued;
    }
}

void HandleNewConnection(std::uint8_t playerId)
{
    if (playerId == 0 || gameState.players.contains(playerId)) {
        return;
    }

    // 승인 여부 1바이트, 참가자 번호 1바이트, UDP 키 8바이트.
    std::vector<BYTE> joinBody(10, 0);

    const auto rejectJoin = [&]() {
        if (!QueueServerTcpPacket(playerId, joinResponseType, joinBody)) {
            RequestDisconnect(playerId);
        }
        };

    if (gameState.phase != lobbyPhase || gameState.players.size() >= 255) {
        rejectJoin();
        return;
    }

    unsigned int high = 0;
    unsigned int low = 0;

    if (rand_s(&high) != 0 || rand_s(&low) != 0) {
        rejectJoin();
        return;
    }

    const std::uint64_t udpKey =
        (static_cast<std::uint64_t>(high) << 32) |
        static_cast<std::uint64_t>(low);

    PlayerState player{};
    player.info.playerId = playerId;
    gameState.players.emplace(playerId, player);

    if (WaitForSingleObject(hConnectionGate, INFINITE) != WAIT_OBJECT_0) {
        gameState.players.erase(playerId);
        return;
    }

    const auto it = connections.find(playerId);
    bool keyStored = false;

    if (it != connections.end() &&
        it->second.state == ConnectionState::Active &&
        it->second.sendContext) {
        try {
            keyStored = issuedUdpKeys.try_emplace(playerId, udpKey).second;
        }
        catch (const std::bad_alloc&) {
            // Gate를 반납한 뒤 입장 실패를 처리한다.
        }
    }

    SetEvent(hConnectionGate);

    if (!keyStored) {
        gameState.players.erase(playerId);
        rejectJoin();
        return;
    }

    joinBody[0] = 1;
    joinBody[1] = playerId;

    for (int i = 0; i < 8; ++i) {
        joinBody[2 + i] = static_cast<BYTE>(udpKey >> (56 - 8 * i));
    }

    if (!QueueServerTcpPacket(playerId, joinResponseType, joinBody)) {
        gameState.players.erase(playerId);
        RequestDisconnect(playerId);
        return;
    }

    std::vector<BYTE> lobbyBody;
    lobbyBody.reserve(1 + gameState.players.size() * 3);
    lobbyBody.push_back(static_cast<BYTE>(gameState.players.size()));

    for (const auto& entry : gameState.players) {
        const auto& info = entry.second.info;
        lobbyBody.push_back(info.playerId);
        lobbyBody.push_back(info.color);
        lobbyBody.push_back(info.ready ? 1 : 0);
    }

    for (const auto& entry : gameState.players) {
        if (!QueueServerTcpPacket(entry.first, lobbyStateType, lobbyBody)) {
            RequestDisconnect(entry.first);
        }
    }
}