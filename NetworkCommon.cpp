#include "NetworkCommon.h"

#include <cstddef>  // size_t   
#include <stdexcept> // std::invalid_argument

#pragma comment(lib, "Ws2_32.lib")

// 소켓에 수신/송신 타임아웃을 적용
bool ApplySocketTimeouts(SOCKET socket,DWORD recvTimeoutMs, DWORD sendTimeoutMs)
{
    const int recvResult = setsockopt(socket, SOL_SOCKET, SO_RCVTIMEO,
        reinterpret_cast<const char*>(&recvTimeoutMs),
        sizeof(recvTimeoutMs));

    if (recvResult == SOCKET_ERROR) {
        return false;
    }

    const int sendResult = setsockopt(
        socket, SOL_SOCKET, SO_SNDTIMEO,
        reinterpret_cast<const char*>(&sendTimeoutMs),
        sizeof(sendTimeoutMs));

    return sendResult == 0;
}

// TCP 패킷을 추출하는 함수 - 덜 받은 데이터는 버퍼에 남겨둠.
std::vector<std::vector<BYTE>> ExtractTcpPackets(std::vector<BYTE>& buffer)
{
    constexpr std::size_t headerSize = 3;
    std::vector<std::vector<BYTE>> packets;
    std::size_t offset = 0;

    while (buffer.size() - offset >= headerSize) {
        // 첫 2바이트에 헤더를 포함한 전체 길이가 들어 있다.
        const std::size_t packetSize =
            (static_cast<std::size_t>(buffer[offset]) << 8) |
            static_cast<std::size_t>(buffer[offset + 1]);

        if (packetSize < headerSize) {
            throw std::invalid_argument("Invalid TCP packet size");
        }

        if (buffer.size() - offset < packetSize) {
            break; // 다음 수신에서 나머지 바이트를 기다린다.
        }

        const auto packetBegin = buffer.begin() + offset;
        packets.emplace_back(packetBegin, packetBegin + packetSize);
        offset += packetSize;
    }

    // 꺼낸 패킷만 지우고 덜 받은 데이터는 남긴다.
    buffer.erase(buffer.begin(), buffer.begin() + offset);
    return packets;
}