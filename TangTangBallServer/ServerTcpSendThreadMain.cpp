#include "ServerTcpSendThreadMain.h"
#include "ServerConnections.h"
#include "TcpSendThread.h"
#include "ServerVariable.h"

DWORD WINAPI ServerTcpSendThreadMain(LPVOID playerIdPtr)
{
    if (playerIdPtr == nullptr)
        return ERROR_INVALID_PARAMETER;

    const BYTE playerId = *static_cast<const BYTE*>(playerIdPtr);
    std::shared_ptr<ServerTcpSendContext> connection;
    if (WaitForSingleObject(hConnectionGate, INFINITE) != WAIT_OBJECT_0)
        return GetLastError();

    const auto it = connections.find(playerId);
    if (it != connections.end() && it->second.state == ConnectionState::Active)
        connection = it->second.sendContext;

    if (!SetEvent(hConnectionGate))
        return GetLastError();
    if (connection == nullptr)
        return ERROR_NOT_FOUND;

    const DWORD result = RunTcpSendLoop(*connection, connection->sendQueue,
        connection->sendQueueGate, connection->sendQueueReady);
    RequestDisconnect(playerId);
    return result;
}
