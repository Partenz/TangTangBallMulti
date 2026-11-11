#include "ClientTcpSendThreadMain.h"
#include "TcpQueue.h"

DWORD WINAPI ClientTcpSendThreadMain(LPVOID parameter)
{
    if (parameter == nullptr)
        return ERROR_INVALID_PARAMETER;

    const auto& connection = *static_cast<TcpSendConnection*>(parameter);
    return RunTcpSendLoop(connection, clientTcpSendQueue, hClientTcpSendGate, hClientTcpSendReady);
}
