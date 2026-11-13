#include <vector>

#include "TcpRecvThread.h"
#include "ClientTcpRecvThreadMain.h"
#include "ClientVariable.h"
#include "NetworkCommon.h"


DWORD WINAPI ClientTcpRecvThreadMain(LPVOID unused)
{
    const SOCKET socket = clientTcpSocket;
    const HANDLE stopEvent = clientTcpConnection.stopEvent;
    const HANDLE disconnectEvent = clientTcpConnection.disconnectEvent;

    if (socket == INVALID_SOCKET || stopEvent == nullptr || disconnectEvent == nullptr) {
        return ERROR_INVALID_PARAMETER;
    }

    const DWORD result = RunTcpReceiveLoop(socket, stopEvent, disconnectEvent);

    // 송신 스레드에 같은 연결의 종료를 알린다.
    const DWORD signalError = SetEvent(disconnectEvent) ? ERROR_SUCCESS : GetLastError();
    shutdown(socket, SD_BOTH);

    return result != ERROR_SUCCESS ? result : signalError;
}