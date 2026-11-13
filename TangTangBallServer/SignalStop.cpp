#include <array>
#include "SignalStop.h"
#include "ServerConnections.h"
#include "ServerVariable.h"

bool SignalStop()
{
    if (!SetEvent(hStopEvent))
        return false;

    std::array<std::uint8_t, 256> playerIds{};
    std::size_t count = 0;
    if (WaitForSingleObject(hConnectionGate, INFINITE) != WAIT_OBJECT_0)
        return false;

    for (const auto& entry : connections)
        playerIds[count++] = entry.first;

    if (!SetEvent(hConnectionGate))
        return false;

    for (std::size_t i = 0; i < count; ++i)
        RequestDisconnect(playerIds[i]);

    return true;
}
