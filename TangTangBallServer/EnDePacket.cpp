#include "EnDePacket.h"
#include <cstdint>
#include <cstring>
#include <cmath>

std::vector<BYTE> EncodePacket(BYTE packetType, const std::vector<BYTE>& body)
{
    if (body.size() > 65535 - 3)
        return {};

    const uint16_t packetSize = static_cast<uint16_t>(body.size() + 3);
    std::vector<BYTE> bytes;
    bytes.reserve(packetSize);
    bytes.push_back(static_cast<BYTE>(packetSize >> 8));
    bytes.push_back(static_cast<BYTE>(packetSize));
    bytes.push_back(packetType);
    bytes.insert(bytes.end(), body.begin(), body.end());
    return bytes;
}

bool DecodePacket(const std::vector<BYTE>& bytes, BYTE& packetType,
    std::vector<BYTE>& body)
{
    // 1. 헤더를 읽기 전에 최소/최대 길이를 확인
    if (bytes.size() < 3 || bytes.size() > 65535)
        return false;

    const uint16_t packetSize = static_cast<uint16_t>(
        (uint16_t(bytes[0]) << 8) | bytes[1]);
    if (packetSize != bytes.size())
        return false;

    const BYTE type = bytes[2];
    const std::vector<BYTE> data(bytes.begin() + 3, bytes.end());
    std::size_t expectedSize = 0;

    // 본문 안의 필드 위치를 기록하고 아래에서 직접 검사
    std::vector<std::size_t> boolPositions;
    std::vector<std::size_t> floatPositions;
    std::vector<std::size_t> objectPositions;

    // 2. 추진계획서 9~11쪽의 번호와 본문 구성을 확인
    switch (type)
    {
    case 1: // JOIN_RESPONSE: result, playerId, udpKey
        expectedSize = 10;
        boolPositions = { 0 };
        break;
    case 2: // LOBBY_STATE: playerCount, PlayerInfo[]
        if (data.empty()) return false;
        expectedSize = 1 + std::size_t(data[0]) * 3;
        if (data.size() != expectedSize) return false;
        for (std::size_t pos = 1; pos < data.size(); pos += 3) {
            if (data[pos] == 0 || data[pos + 1] > 5) return false;
            boolPositions.push_back(pos + 2);
        }
        break;
    case 3: // COLOR_REQUEST: color (6가지 색상은 0~5로 표현)
        expectedSize = 1;
        if (data.size() != 1 || data[0] > 5) return false;
        break;
    case 4: // COLOR_RESPONSE: result
    case 5: // READY_REQUEST: ready
    case 6: // READY_RESPONSE: result
    case 23: // PLACE_RESPONSE: result
    case 25: // RACE_BEGIN: electricityActive
        expectedSize = 1;
        boolPositions = { 0 };
        break;
    case 7: // LEAVE_REQUEST
    case 11: // MAP_READY
    case 42: // RETURN_LOBBY
    case 101: // UDP_REGISTERED
        expectedSize = 0;
        break;
    case 8: // PLAYER_LEAVE: playerId
        expectedSize = 1;
        if (data.size() != 1 || data[0] == 0) return false;
        break;
    case 10: // MAP_DATA: width, height, objectCount, spawnX, spawnY, count, objects[]
    {
        if (data.size() < 18) return false;
        const uint32_t objectCount = (uint32_t(data[4]) << 24)
            | (uint32_t(data[5]) << 16) | (uint32_t(data[6]) << 8) | data[7];
        const uint16_t count = static_cast<uint16_t>(
            (uint16_t(data[16]) << 8) | data[17]);
        expectedSize = 18 + std::size_t(count) * 22;
        if (count > objectCount || data.size() != expectedSize) return false;
        floatPositions = { 8, 12 };
        for (std::size_t pos = 18; pos < data.size(); pos += 22)
            objectPositions.push_back(pos);
        break;
    }
    case 20: // ROUND_BEGIN: roundCnt, spawnX, spawnY
        expectedSize = 10;
        floatPositions = { 2, 6 };
        break;
    case 21: // BLOCK_ASSIGN: blockType (기존 프로젝트의 블록 번호 0~8)
        expectedSize = 1;
        if (data.size() != 1 || data[0] > 8) return false;
        break;
    case 22: // PLACE_REQUEST: gridX:int16, gridY:int16
        expectedSize = 4;
        break;
    case 24: // BLOCK_PLACED: playerId, ObjectInfo
        expectedSize = 23;
        if (data.size() != expectedSize || data[0] == 0) return false;
        objectPositions = { 1 };
        break;
    case 30: // CONTACT_EVENT: roundCnt, playerId, objectId, contactType
        expectedSize = 8;
        if (data.size() != expectedSize || data[2] == 0 || data[7] > 4)
            return false;
        break;
    case 31: // DOUBLE_JUMP_REQUEST: roundCnt, direction
        expectedSize = 3;
        break;
    case 40: // ROUND_RESULT: roundCnt, winnerId, ScoreInfo[]
    case 41: // GAME_RESULT: winnerId, ScoreInfo[]
    {
        const std::size_t prefixSize = type == 40 ? 3 : 1;
        if (data.size() < prefixSize || (data.size() - prefixSize) % 2 != 0)
            return false;
        // 별도 개수 필드가 없으므로 남은 바이트를 playerId/score 쌍으로 검사
        for (std::size_t pos = prefixSize; pos < data.size(); pos += 2)
            if (data[pos] == 0) return false;
        expectedSize = data.size();
        break;
    }
    case 100: // UDP_REGISTER: playerId, udpKey
        expectedSize = 9;
        if (data.size() != expectedSize || data[0] == 0) return false;
        break;
    case 110: // MOVE_INPUT: roundCnt, udpSequence, moveDirection
        expectedSize = 7;
        break;
    case 111: // PLAYERS_STATE: roundCnt, udpSequence, playerCount, PlayerMotion[]
        if (data.size() < 7) return false;
        expectedSize = 7 + std::size_t(data[6]) * 18;
        if (data.size() != expectedSize) return false;
        for (std::size_t pos = 7; pos < data.size(); pos += 18) {
            if (data[pos] == 0) return false;
            for (std::size_t offset = 1; offset <= 13; offset += 4)
                floatPositions.push_back(pos + offset);
            boolPositions.push_back(pos + 17);
        }
        break;
    case 120: // PREVIEW_STATE: roundCnt, udpSequence, gridX, gridY, visible
        expectedSize = 11;
        boolPositions = { 10 };
        break;
    case 121: // PLAYERS_PREVIEW: roundCnt, udpSequence, playerId, blockType, gridX, gridY, visible
        expectedSize = 13;
        if (data.size() != expectedSize || data[6] == 0 || data[7] > 8)
            return false;
        boolPositions = { 12 };
        break;
    default:
        return false;
    }

    // 3. 본문 길이가 정확한지 확인한 뒤 개별 필드 형식을 검사
    if (data.size() != expectedSize)
        return false;

    // ObjectInfo: id(4), type(1), 좌표 int32 네 개(16), used(1).
    for (std::size_t pos : objectPositions) {
        if (data[pos + 4] > 8) return false;
        boolPositions.push_back(pos + 21);
    }

    for (std::size_t pos : boolPositions)
        if (data[pos] > 1) return false;

    for (std::size_t pos : floatPositions) {
        const uint32_t bits = (uint32_t(data[pos]) << 24)
            | (uint32_t(data[pos + 1]) << 16)
            | (uint32_t(data[pos + 2]) << 8) | data[pos + 3];
        float value;
        std::memcpy(&value, &bits, sizeof(value));
        if (!std::isfinite(value)) return false;
    }

    // 4. 모든 검사가 성공했을 때만 출력, 게임 상태는 변경 X
    body = data;
    packetType = type;
    return true;
}