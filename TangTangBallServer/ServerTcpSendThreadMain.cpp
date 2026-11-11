#include "ServerTcpSendThreadMain.h"

DWORD WINAPI ServerTcpSendThreadMain(LPVOID parameter)
{
    if (parameter == nullptr)
        return ERROR_INVALID_PARAMETER;

    auto& connection = *static_cast<ServerTcpSendContext*>(parameter);
    return RunTcpSendLoop(connection, connection.sendQueue, connection.sendQueueGate, connection.sendQueueReady);
}
