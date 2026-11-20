#include <algorithm>
#include <cstdint>
#include <limits>
#include <vector>

#include "../BlockAction.h"
#include "HandlePlace.h"
#include "EnDePacket.h"
#include "EventQueue.h"
#include "ServerConnections.h"
#include "ServerVariable.h"

extern GameState gameState;

namespace {
    void AppendUint32(std::vector<BYTE>& body, uint32_t value)
    {
        body.push_back(static_cast<BYTE>(value >> 24));
        body.push_back(static_cast<BYTE>(value >> 16));
        body.push_back(static_cast<BYTE>(value >> 8));
        body.push_back(static_cast<BYTE>(value));
    }

    void SendPacket(uint8_t playerId, BYTE packetType, const std::vector<BYTE>& body)
    {
        NetEvent event{};
        event.playerId = playerId;
        event.packetType = packetType;
        event.packetData = EncodePacket(packetType, body);
        if (event.packetData.empty() ||
            WaitForSingleObject(hConnectionGate, INFINITE) != WAIT_OBJECT_0)
            return;

        const auto connection = connections.find(playerId);
        if (connection == connections.end() ||
            connection->second.state != ConnectionState::Active ||
            !connection->second.sendContext) {
            SetEvent(hConnectionGate);
            return;
        }

        auto& context = *connection->second.sendContext;
        const bool queued = PushEventQueue(
            context.sendQueue, event,
            context.sendQueueGate, context.sendQueueReady);

        SetEvent(hConnectionGate);
        if (!queued)
            RequestDisconnect(playerId);
    }

    constexpr BYTE placingPhase = 2;
    constexpr BYTE placeResponseType = 23;
    constexpr BYTE blockPlacedType = 24;
    constexpr int gridSpacing = 40;
    constexpr BYTE thornBlockType = 1;
}

void HandlePlace(uint8_t playerId, int16_t gridX, int16_t gridY, bool deleteRequested)
{
    const auto player = gameState.players.find(playerId);
    if (player == gameState.players.end())
        return;

    // 패킷의 격자 인덱스를 게임에서 사용하는 픽셀 좌표로 바꾼다.
    const int left = gridX * gridSpacing;
    const int top = gridY * gridSpacing;
    const int right = left + gridSpacing;
    const int bottom = top + gridSpacing;

    if (gameState.phase != placingPhase || player->second.placed ||
        deleteRequested != (player->second.assignedBlockType == BlockAction::deleteAssignment) ||
        left < 0 || top < 0 || right > gameState.mapWidth || bottom > gameState.mapHeight) {
        SendPacket(playerId, placeResponseType, { 0 });
        return;
    }

    const auto target = std::find_if(gameState.objects.begin(), gameState.objects.end(),
        [&](const ObjectInfo& object) {
            return (!deleteRequested || object.blockType != BlockAction::starBlock) &&
                left < object.right && right > object.left &&
                top < object.bottom && bottom > object.top;
        });

    if (deleteRequested) {
        if (target == gameState.objects.end()) {
            SendPacket(playerId, placeResponseType, { 0 });
            return;
        }

        std::vector<BYTE> deletedBody{ playerId };
        AppendUint32(deletedBody, target->objectId);
        gameState.objects.erase(target);
        player->second.placed = true;
        player->second.previewVisible = false;
        SendPacket(playerId, placeResponseType, { 1 });
        for (const auto& entry : gameState.players)
            SendPacket(entry.first, BlockAction::blockDeleted, deletedBody);
        return;
    }

    // 맵에 이미 저장된 번호와 겹치지 않도록 다음 번호를 보정한다.
    constexpr uint32_t maxObjectId = (std::numeric_limits<uint32_t>::max)();
    uint32_t objectId = gameState.nextObjectId;
    for (const ObjectInfo& object : gameState.objects) {
        if (object.objectId >= objectId)
            objectId = object.objectId == maxObjectId ? maxObjectId : object.objectId + 1;
    }

    if (target != gameState.objects.end() || objectId == maxObjectId) {
        SendPacket(playerId, placeResponseType, { 0 });
        return;
    }

    const BYTE blockType = player->second.assignedBlockType;
    const int blockTop = blockType == thornBlockType ? top + gridSpacing / 2 : top;
    const ObjectInfo object{ objectId, blockType, left, blockTop, right, bottom, false };

    std::vector<BYTE> placedBody = { playerId };
    AppendUint32(placedBody, object.objectId);
    placedBody.push_back(object.blockType);
    AppendUint32(placedBody, static_cast<uint32_t>(object.left));
    AppendUint32(placedBody, static_cast<uint32_t>(object.top));
    AppendUint32(placedBody, static_cast<uint32_t>(object.right));
    AppendUint32(placedBody, static_cast<uint32_t>(object.bottom));
    placedBody.push_back(static_cast<BYTE>(object.used));

    gameState.objects.push_back(object);
    gameState.nextObjectId = objectId + 1;
    player->second.placed = true;
    player->second.previewVisible = false;

    SendPacket(playerId, placeResponseType, { 1 });
    for (const auto& entry : gameState.players)
        SendPacket(entry.first, blockPlacedType, placedBody);
}
