#include <bit>
#include <cstddef>
#include <filesystem>
#include <fstream>
#include <iostream>
#include <random>
#include <string>
#include <utility>
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

    constexpr BYTE lobbyPhase = 0;
    constexpr BYTE loadingPhase = 1;
    if (gameState.phase == lobbyPhase) {
        // 첫 MAP_DATA를 보내기 전에 이번 경기의 맵을 한 번만 고른다.
        static std::mt19937 randomEngine{ std::random_device{}() };
        const int stageNumber = std::uniform_int_distribution<int>{ 1, 3 }(randomEngine);
        const std::string stageFileName = "stage" + std::to_string(stageNumber) + ".txt";

        wchar_t executablePath[MAX_PATH]{};
        const DWORD pathLength = GetModuleFileNameW(nullptr, executablePath, MAX_PATH);
        if (pathLength == 0 || pathLength >= MAX_PATH) {
            std::cerr << stageFileName << ": server executable path unavailable\n";
            return false;
        }

        std::ifstream mapFile(std::filesystem::path(executablePath).parent_path() / stageFileName);
        int objectCount = 0;
        int starCount = 0;
        float spawnX = 0.0f;
        float spawnY = 0.0f;
        if (!(mapFile >> objectCount >> starCount >> spawnX >> spawnY) ||
            objectCount <= 0 || static_cast<std::size_t>(objectCount) > maxObjectCount || starCount != 1) {
            std::cerr << stageFileName << ": invalid or missing map header\n";
            return false;
        }

        std::vector<ObjectInfo> mapObjects;
        mapObjects.reserve(objectCount);
        uint16_t mapWidth = 0;
        uint16_t mapHeight = 0;
        for (int index = 0; index < objectCount; ++index) {
            ObjectInfo object{};
            int blockType = 0;
            if (!(mapFile >> blockType >> object.left >> object.top >> object.right >> object.bottom) ||
                blockType < 0 || blockType > 8 || object.left < 0 || object.top < 0 ||
                object.left >= object.right || object.top >= object.bottom ||
                object.right > 65535 || object.bottom > 65535) {
                std::cerr << stageFileName << ": invalid object " << index + 1 << '\n';
                return false;
            }

            object.objectId = index + 1;
            object.blockType = static_cast<BYTE>(blockType);
            mapObjects.push_back(object);
            if (object.right > mapWidth)
                mapWidth = static_cast<uint16_t>(object.right);
            if (object.bottom > mapHeight)
                mapHeight = static_cast<uint16_t>(object.bottom);
        }

        gameState.baseMapObjects = mapObjects;
        gameState.objects = std::move(mapObjects);
        gameState.mapWidth = mapWidth;
        gameState.mapHeight = mapHeight;
        gameState.spawnX = spawnX;
        gameState.spawnY = spawnY;
        gameState.nextObjectId = static_cast<std::uint32_t>(objectCount) + 1;
        for (auto& entry : gameState.players)
            entry.second.mapReady = false;

        gameState.phase = loadingPhase;
        std::cout << "Selected map: " << stageFileName << '\n';
    }

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