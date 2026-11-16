#include <bit>
#include <cstddef>
#include <vector>

#include "SendMap.h"
#include "ServerConnections.h"
#include "ServerVariable.h"

extern GameState gameState;

// 서버에서 데이터 값을 클라가 읽을 수 있는 바이트 순서로 보내기 위해서 만듦
namespace {
    // 맵 크기, 패킷의 객체 수를 보내기 위해
    void AppendUint16(std::vector<BYTE>& body, std::uint16_t value)
    {
        body.push_back(static_cast<BYTE>(value >> 8));
        body.push_back(static_cast<BYTE>(value));
    }

    // 전체 객체 수, 좌표등 4바이트 값을 보내기 위해
    void AppendUint32(std::vector<BYTE>& body, std::uint32_t value)
    {
        body.push_back(static_cast<BYTE>(value >> 24));
        body.push_back(static_cast<BYTE>(value >> 16));
        body.push_back(static_cast<BYTE>(value >> 8));
        body.push_back(static_cast<BYTE>(value));
    }
}

bool SendMap(std::uint8_t playerId)
{
    constexpr BYTE mapDataType = 10;
    constexpr std::size_t headerSize = 3;
    constexpr std::size_t mapFieldsSize = 18;
    constexpr std::size_t objectSize = 22;
    constexpr std::size_t maxObjectCount = (65535 - headerSize - mapFieldsSize) / objectSize;

    const auto& objects = gameState.objects;
    const auto objectCount = objects.size();
    if (objectCount > maxObjectCount) {
        return false;
    }

    std::vector<BYTE> body;
    body.reserve(mapFieldsSize + objectCount * objectSize);

    AppendUint16(body, gameState.mapWidth);
    AppendUint16(body, gameState.mapHeight);
    AppendUint32(body, static_cast<std::uint32_t>(objectCount));
    AppendUint32(body, std::bit_cast<std::uint32_t>(gameState.spawnX));
    AppendUint32(body, std::bit_cast<std::uint32_t>(gameState.spawnY));
    AppendUint16(body, static_cast<std::uint16_t>(objectCount));

    for (const auto& object : objects) {
        AppendUint32(body, object.objectId);
        body.push_back(object.blockType);
        AppendUint32(body, static_cast<std::uint32_t>(object.left));
        AppendUint32(body, static_cast<std::uint32_t>(object.top));
        AppendUint32(body, static_cast<std::uint32_t>(object.right));
        AppendUint32(body, static_cast<std::uint32_t>(object.bottom));
        body.push_back(static_cast<BYTE>(object.used));
    }

    return QueueServerTcpPacket(playerId, mapDataType, body);
}