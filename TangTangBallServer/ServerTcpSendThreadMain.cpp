#include "ServerTcpSendThreadMain.h"
#include "TcpSendThread.h"
#include "ServerVariable.h"

DWORD WINAPI ServerTcpSendThreadMain(LPVOID playerIdPtr)
{
    if (playerIdPtr == nullptr)
        return ERROR_INVALID_PARAMETER;

    const BYTE playerId = *static_cast<const BYTE*>(playerIdPtr);
    ServerTcpSendContext* connection = nullptr;
    if (WaitForSingleObject(hConnectionGate, INFINITE) != WAIT_OBJECT_0)
        return GetLastError();

    const auto it = serverTcpConnections.find(playerId);
    if (it != serverTcpConnections.end())
        connection = it->second;

    if (!SetEvent(hConnectionGate))
        return GetLastError();
    if (connection == nullptr)
        return ERROR_NOT_FOUND;

    return RunTcpSendLoop(*connection, connection->sendQueue,
        connection->sendQueueGate, connection->sendQueueReady);
}
