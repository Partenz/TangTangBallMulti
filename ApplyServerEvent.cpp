#include <algorithm>
#include <bitset>
#include <cstdint>
#include <cstring>
#include <utility>
#include <vector>

#include "BlockAction.h"
#include "ApplyServerEvent.h"
#include "TCPQueue.h"
#include "resource.h"
#include <mmsystem.h>


namespace {
    constexpr BYTE lobbyPhase = 0;
    constexpr BYTE loadingPhase = 1;
    constexpr BYTE placingPhase = 2;
    constexpr BYTE racingPhase = 3;
    constexpr BYTE roundResultPhase = 4;
    constexpr BYTE gameResultPhase = 5;

    template<typename UInt>
    UInt ReadUnsigned(const std::vector<BYTE>& body, std::size_t offset)
    {
        UInt value = 0;
        for (std::size_t i = 0; i < sizeof(UInt); ++i)
            value = static_cast<UInt>((value << 8) | body[offset + i]);
        return value;
    }

    float ReadFloat(const std::vector<BYTE>& body, std::size_t offset)
    {
        const uint32_t bits = ReadUnsigned<uint32_t>(body, offset);
        float value;
        std::memcpy(&value, &bits, sizeof(value));
        return value;
    }

    ObjectInfo ReadObject(const std::vector<BYTE>& body)
    {
        return {
            ReadUnsigned<uint32_t>(body, 1),
            body[5],
            static_cast<int32_t>(ReadUnsigned<uint32_t>(body, 6)),
            static_cast<int32_t>(ReadUnsigned<uint32_t>(body, 10)),
            static_cast<int32_t>(ReadUnsigned<uint32_t>(body, 14)),
            static_cast<int32_t>(ReadUnsigned<uint32_t>(body, 18)),
            body[22] != 0
        };
    }

    void ResetPlayerForRound(PlayerState& player, float spawnX, float spawnY)
    {
        const PlayerInfo info = player.info;
        const uint8_t score = player.score;
        const bool mapReady = player.mapReady;
        player = {};
        player.info = info;
        player.score = score;
        player.mapReady = mapReady;
        player.assignedBlockType = 0xff; // No block until BLOCK_ASSIGN arrives.
        player.alive = true;
        player.motion = { info.playerId, spawnX, spawnY, 0.0f, 0.0f, false };
    }
}

bool ApplyMapData(const std::vector<BYTE>& body)
{
    constexpr std::size_t mapFieldsSize = 18;
    constexpr std::size_t objectSize = 22;
    constexpr BYTE mapReadyType = 11;

    if (body.size() < mapFieldsSize) {
        return false;
    }

    const auto width = ReadUnsigned<std::uint16_t>(body, 0);
    const auto height = ReadUnsigned<std::uint16_t>(body, 2);
    const auto objectCount = ReadUnsigned<std::uint32_t>(body, 4);
    const auto count = ReadUnsigned<std::uint16_t>(body, 16);

    // 현재 SendMap()은 맵 전체를 패킷 하나에 담는다.
    if (width == 0 || height == 0 || objectCount != count ||
        body.size() != mapFieldsSize + std::size_t(count) * objectSize) {
        return false;
    }

    std::vector<ObjectInfo> objects;
    objects.reserve(count);

    for (std::size_t offset = mapFieldsSize; offset < body.size(); offset += objectSize) {
        ObjectInfo object{};
        object.objectId = ReadUnsigned<std::uint32_t>(body, offset);
        object.blockType = body[offset + 4];
        object.left = static_cast<std::int32_t>(ReadUnsigned<std::uint32_t>(body, offset + 5));
        object.top = static_cast<std::int32_t>(ReadUnsigned<std::uint32_t>(body, offset + 9));
        object.right = static_cast<std::int32_t>(ReadUnsigned<std::uint32_t>(body, offset + 13));
        object.bottom = static_cast<std::int32_t>(ReadUnsigned<std::uint32_t>(body, offset + 17));
        object.used = body[offset + 21] != 0;
        objects.push_back(object);
    }

    clientGameState.mapWidth = width;
    clientGameState.mapHeight = height;
    clientGameState.spawnX = ReadFloat(body, 8);
    clientGameState.spawnY = ReadFloat(body, 12);
    clientGameState.objects = std::move(objects);

    return QueueTcpPacket(mapReadyType, {});
}

void ApplyServerEvent(const NetEvent& event)
{
    if (event.eventKind != NetEventPacket)
        return;

    // The receive thread has already validated the wire packet with DecodePacket.
    const auto& body = event.packetData;
    auto& state = clientGameState;

    switch (event.packetType) {
    case 2: { // LOBBY_STATE
        if (state.phase != lobbyPhase || body.empty() || body.size() != 1 + std::size_t(body[0]) * 3)
            return;

        std::unordered_map<uint8_t, PlayerState> players;
        for (std::size_t offset = 1; offset < body.size(); offset += 3) {
            const uint8_t playerId = body[offset];
            PlayerState player{};
            player.info = { playerId, body[offset + 1], body[offset + 2] != 0 };
            player.motion.playerId = playerId;
            players.emplace(playerId, player);
        }
        state.players = std::move(players);
        break;
    }
    case 8: // PLAYER_LEAVE
        if (body.size() == 1)
            state.players.erase(body[0]);
        break;

    case 10: // MAP_DATA
        if ((state.phase == lobbyPhase || state.phase == loadingPhase) &&
            body.size() >= 18 && ApplyMapData(body)) {
            state.phase = loadingPhase;
        }
        break;

    case 20: // ROUND_BEGIN
        if (body.size() != 10)
            return;

        state.roundCnt = ReadUnsigned<uint16_t>(body, 0);
        state.spawnX = ReadFloat(body, 2);
        state.spawnY = ReadFloat(body, 6);
        state.phase = placingPhase;
        state.roundWinnerId = 0;
        state.electricityActive = false;
        for (auto& object : state.objects)
            object.used = false;
        for (auto& entry : state.players)
            ResetPlayerForRound(entry.second, state.spawnX, state.spawnY);
        break;

    case 21: { // BLOCK_ASSIGN
        if (body.size() != 1 || state.phase != placingPhase)
            return;
        const auto player = state.players.find(clientPlayerId);
        if (player != state.players.end())
            player->second.assignedBlockType = body[0];
        break;
    }
    case 23: { // PLACE_RESPONSE
        if (body.size() != 1 || body[0] != 1 || state.phase != placingPhase)
            return;
        const auto player = state.players.find(clientPlayerId);
        if (player != state.players.end()) {
            player->second.placed = true;
            player->second.previewVisible = false;
        }
        break;
    }
    case 24: { // BLOCK_PLACED
        if (body.size() != 23)
            return;

        const ObjectInfo object = ReadObject(body);
        const auto existing = std::find_if(state.objects.begin(), state.objects.end(),
            [&](const ObjectInfo& entry) { return entry.objectId == object.objectId; });
        if (existing == state.objects.end())
            state.objects.push_back(object);
        else
            *existing = object;

        const auto player = state.players.find(body[0]);
        if (player != state.players.end()) {
            player->second.placed = true;
            player->second.previewVisible = false;
        }
        break;
    }
    case BlockAction::blockDeleted: {
        if (body.size() != 5 || state.phase != placingPhase)
            return;
        const auto objectId = ReadUnsigned<uint32_t>(body, 1);
        const auto target = std::find_if(state.objects.begin(), state.objects.end(),
            [objectId](const ObjectInfo& object) { return object.objectId == objectId; });
        if (target != state.objects.end())
            state.objects.erase(target);
        const auto player = state.players.find(body[0]);
        if (player != state.players.end()) {
            player->second.placed = true;
            player->second.previewVisible = false;
        }
        break;
    }
    case 25: // RACE_BEGIN
        if (body.size() != 1)
            return;
        state.phase = racingPhase;
        state.electricityActive = body[0] != 0;
        for (auto& entry : state.players)
            entry.second.previewVisible = false;
        break;

    case 30: { // CONTACT_EVENT
        if (body.size() != 8 || state.phase != racingPhase ||
            ReadUnsigned<uint16_t>(body, 0) != state.roundCnt)
            return;

        const uint8_t playerId = body[2];
        const uint32_t objectId = ReadUnsigned<uint32_t>(body, 3);
        const uint8_t contactType = body[7];
        const auto player = state.players.find(playerId);
        int soundResource = 0;

        switch (contactType) {
        case 0: // Thorn
        case 1: // Electricity
            if (player != state.players.end()) {
                player->second.alive = false;
                player->second.doubleJumpReady = false;
            }
            soundResource = IDW_DEAD;
            break;
        case 2: // Single-use block
        case 3: // Double jump item
        case 4: { // Goal
            const auto object = std::find_if(state.objects.begin(), state.objects.end(),
                [objectId](const ObjectInfo& entry) { return entry.objectId == objectId; });
            if (object != state.objects.end())
                object->used = true;
            if (contactType == 3 && player != state.players.end())
                player->second.doubleJumpReady = true;
            if (contactType == 2)
                soundResource = IDW_ONCE_BLOCK;
            if (contactType == 4) {
                state.roundWinnerId = playerId;
                soundResource = IDW_STAR;
            }
            break;
        }
        }
        if (soundResource != 0)
            PlaySoundW(MAKEINTRESOURCEW(soundResource), GetModuleHandleW(nullptr), SND_ASYNC | SND_RESOURCE | SND_NODEFAULT);
        break;
    }
    case 40: // ROUND_RESULT
    case 41: { // GAME_RESULT
        const bool roundResult = event.packetType == 40;
        const std::size_t scoreOffset = roundResult ? 3 : 1;
        if (body.size() < scoreOffset || (body.size() - scoreOffset) % 2 != 0)
            return;
        if (roundResult && ReadUnsigned<uint16_t>(body, 0) != state.roundCnt)
            return;

        for (std::size_t offset = scoreOffset; offset < body.size(); offset += 2) {
            const auto player = state.players.find(body[offset]);
            if (player != state.players.end())
                player->second.score = body[offset + 1];
        }
        state.roundWinnerId = body[scoreOffset - 1];
        state.phase = roundResult ? roundResultPhase : gameResultPhase;
        break;
    }
    case 42: { // RETURN_LOBBY
        if (!body.empty())
            return;

        auto players = std::move(state.players);
        state = {};
        for (auto& entry : players) {
            PlayerInfo info = entry.second.info;
            info.ready = false;
            entry.second = {};
            entry.second.info = info;
            entry.second.motion.playerId = info.playerId;
        }
        state.players = std::move(players);
        break;
    }
    case 111: { // PLAYERS_STATE
        if (body.size() < 7 || body.size() != 7 + std::size_t(body[6]) * 18 ||
            state.phase != racingPhase || ReadUnsigned<uint16_t>(body, 0) != state.roundCnt)
            return;

        std::bitset<256> alivePlayers;
        for (std::size_t offset = 7; offset < body.size(); offset += 18) {
            const uint8_t playerId = body[offset];
            alivePlayers.set(playerId);
            const auto player = state.players.find(playerId);
            if (player == state.players.end() || !player->second.alive)
                continue;

            player->second.motion = {
                playerId,
                ReadFloat(body, offset + 1),
                ReadFloat(body, offset + 5),
                ReadFloat(body, offset + 9),
                ReadFloat(body, offset + 13),
                body[offset + 17] != 0
            };
            player->second.lastMoveTime = GetTickCount64();
        }
        // The server sends a complete list of survivors, including after a fall off the map.
        for (auto& [playerId, player] : state.players) {
            if (!alivePlayers.test(playerId)) {
                player.alive = false;
                player.doubleJumpReady = false;
            }
        }
        break;
    }
    case 121: { // PLAYERS_PREVIEW
        if (body.size() != 13 || state.phase != placingPhase ||
            ReadUnsigned<uint16_t>(body, 0) != state.roundCnt)
            return;

        const auto player = state.players.find(body[6]);
        if (player != state.players.end() && !player->second.placed) {
            player->second.assignedBlockType = body[7];
            player->second.previewGridX = static_cast<int16_t>(ReadUnsigned<uint16_t>(body, 8));
            player->second.previewGridY = static_cast<int16_t>(ReadUnsigned<uint16_t>(body, 10));
            player->second.previewVisible = body[12] != 0;
        }
        break;
    }
    }
}
