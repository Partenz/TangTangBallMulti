#include <algorithm>

#include "../BlockAction.h"
#include "HandlePreview.h"
#include "EnDePacket.h"
#include "EventQueue.h"
#include "IsNewerSequence.h"
#include "ServerVariable.h"

void HandlePreview(std::uint8_t playerId, std::uint16_t roundCnt, std::uint32_t sequence, std::int16_t gridX, std::int16_t gridY, bool visible)
{
    constexpr std::uint8_t placingPhase = 2;
    constexpr BYTE playersPreviewPacketType = 121;
    constexpr int gridSpacing = 40;

    if (gameState.phase != placingPhase || roundCnt != gameState.roundCnt)
        return;

    const auto playerEntry = gameState.players.find(playerId);
    if (playerEntry == gameState.players.end() || playerEntry->second.placed)
        return;

    std::uint64_t senderUdpKey = 0;
    std::vector<std::uint8_t> recipientPlayerIds;
    recipientPlayerIds.reserve(gameState.players.size());

    if (WaitForSingleObject(hConnectionGate, INFINITE) != WAIT_OBJECT_0)
        return;

    const auto senderKeyEntry = issuedUdpKeys.find(playerId);
    const bool hasSenderUdpKey = senderKeyEntry != issuedUdpKeys.end();
    if (hasSenderUdpKey) {
        senderUdpKey = senderKeyEntry->second;

        for (const auto& recipientEntry : gameState.players) {
            const std::uint8_t recipientPlayerId = recipientEntry.first;
            if (recipientPlayerId != playerId && registeredUdpAddresses.contains(recipientPlayerId))
                recipientPlayerIds.push_back(recipientPlayerId);
        }
    }

    if (!SetEvent(hConnectionGate) || !hasSenderUdpKey)
        return;

    const auto sequenceEntry = gameState.lastPreviewSequences.find(playerId);
    if (sequenceEntry != gameState.lastPreviewSequences.end()) {
        const auto& [lastUdpKey, lastSequence] = sequenceEntry->second;
        if (lastUdpKey == senderUdpKey && !IsNewerSequence(sequence, lastSequence))
            return;
    }

    // 숨김 요청은 좌표와 관계없이 처리한다.
    if (visible) {
        const int left = gridX * gridSpacing;
        const int top = gridY * gridSpacing;
        const int right = left + gridSpacing;
        const int bottom = top + gridSpacing;

        if (left < 0 || top < 0 || right > gameState.mapWidth || bottom > gameState.mapHeight)
            return;

        const bool deleteAssigned = playerEntry->second.assignedBlockType == BlockAction::deleteAssignment;
        const bool hasTarget = std::any_of(gameState.objects.begin(), gameState.objects.end(),
            [&](const ObjectInfo& object) {
                return (!deleteAssigned || object.blockType != BlockAction::starBlock) &&
                    left < object.right && right > object.left && top < object.bottom && bottom > object.top;
            });
        if (deleteAssigned != hasTarget)
            return;
    }

    gameState.lastPreviewSequences.insert_or_assign(playerId, std::pair{ senderUdpKey, sequence });

    PlayerState& player = playerEntry->second;
    player.previewGridX = gridX;
    player.previewGridY = gridY;
    player.previewVisible = visible;

    const std::uint32_t udpSequence = ++serverUdpSendSequence;
    const auto gridXBits = static_cast<std::uint16_t>(gridX);
    const auto gridYBits = static_cast<std::uint16_t>(gridY);

    const std::vector<BYTE> previewBody = {
        static_cast<BYTE>(roundCnt >> 8),
        static_cast<BYTE>(roundCnt),
        static_cast<BYTE>(udpSequence >> 24),
        static_cast<BYTE>(udpSequence >> 16),
        static_cast<BYTE>(udpSequence >> 8),
        static_cast<BYTE>(udpSequence),
        playerId,
        player.assignedBlockType,
        static_cast<BYTE>(gridXBits >> 8),
        static_cast<BYTE>(gridXBits),
        static_cast<BYTE>(gridYBits >> 8),
        static_cast<BYTE>(gridYBits),
        static_cast<BYTE>(visible)
    };

    NetEvent sendEvent{};
    sendEvent.packetType = playersPreviewPacketType;
    sendEvent.packetData = EncodePacket(playersPreviewPacketType, previewBody);

    for (std::uint8_t recipientPlayerId : recipientPlayerIds) {
        sendEvent.playerId = recipientPlayerId;
        if (!PushEventQueue(serverUdpSendQueue, sendEvent, hServerUdpSendGate, hServerUdpSendReady))
            return;
    }
}