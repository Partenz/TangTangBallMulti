#include "ClientTcpSendThreadMain.h"
#include "TcpSendThread.h"
#include "ClientVariable.h"

DWORD WINAPI ClientTcpSendThreadMain(LPVOID unused)
{
    return RunTcpSendLoop(clientTcpConnection, clientTcpSendQueue, hClientTcpSendGate, hClientTcpSendReady);
}
