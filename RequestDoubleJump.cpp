#include "RequestDoubleJump.h"
#include "TCPQueue.h"


bool RequestDoubleJump(std::uint16_t roundCnt, std::uint8_t direction)
{
    constexpr BYTE doubleJumpRequestPacketType = 31;
    constexpr std::uint8_t leftDirection = 1;
    constexpr std::uint8_t rightDirection = 2;

    if (direction != leftDirection && direction != rightDirection) {
        return false;
    }

    const std::vector<BYTE> doubleJumpBody = { static_cast<BYTE>(roundCnt >> 8), static_cast<BYTE>(roundCnt), direction };

    return QueueTcpPacket(doubleJumpRequestPacketType, doubleJumpBody);
}