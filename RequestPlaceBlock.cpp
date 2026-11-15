#include <cstdint>
#include <vector>

#include "RequestPlaceBlock.h"
#include "TCPQueue.h"

bool RequestPlaceBlock(int16_t gridX, int16_t gridY)
{
    constexpr BYTE placeRequestType = 22;
    const uint16_t x = static_cast<uint16_t>(gridX);
    const uint16_t y = static_cast<uint16_t>(gridY);
    const std::vector<BYTE> body = {
        static_cast<BYTE>(x >> 8), static_cast<BYTE>(x),
        static_cast<BYTE>(y >> 8), static_cast<BYTE>(y)
    };

    return QueueTcpPacket(placeRequestType, body);
}
