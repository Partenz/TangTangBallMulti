#include "TcpQueue.h"
#include "EnDePacket.h"
#include <new>

bool QueueTcpPacket(BYTE packetType, const std::vector<BYTE>& body)
{
    try {
        NetEvent event;
        event.packetType = packetType;
        event.packetData = EncodePacket(packetType, body);
        if (event.packetData.empty()) {
            SetLastError(ERROR_INVALID_DATA);
            return false;
        }

        return PushEventQueue(clientTcpSendQueue, event,
            hClientTcpSendGate, hClientTcpSendReady);
    }
    catch (const std::bad_alloc&) {
        SetLastError(ERROR_NOT_ENOUGH_MEMORY);
        return false;
    }
}
