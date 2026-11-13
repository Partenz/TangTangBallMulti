#include <vector>

#include "ServerTcpRecvThreadMain.h"
#include "ServerConnections.h"
#include "NetworkCommon.h"
#include "EnDePacket.h"
#include "EventQueue.h"
#include "TcpRecvThread.h"


DWORD WINAPI ServerTcpRecvThreadMain(LPVOID playerIdPtr)
{
    if (playerIdPtr == nullptr) {
        return ERROR_INVALID_PARAMETER;
    }

    const BYTE playerId = *static_cast<const BYTE*>(playerIdPtr);

    if (WaitForSingleObject(hConnectionGate, INFINITE) != WAIT_OBJECT_0) {
        return GetLastError();
    }

    SOCKET socket = INVALID_SOCKET;
    HANDLE disconnectEvent = nullptr;

    const auto it = connections.find(playerId);
    if (it != connections.end() && it->second.state == ConnectionState::Active) {
        socket = it->second.socket;
        disconnectEvent = it->second.disconnectEvent;
    }

    // 소켓에서 데이터를 기다리기 전에 연결 목록의 Gate를 반납한다.
    if (!SetEvent(hConnectionGate)) {
        return GetLastError();
    }
    if (socket == INVALID_SOCKET) {
        return ERROR_NOT_FOUND;
    }

    const DWORD result = RunTcpReceiveLoop(socket, disconnectEvent, playerId);

    // 상대가 연결을 닫거나 수신이 끝나면 서버 연결의 종료를 요청한다.
    RequestDisconnect(playerId);
    return result;
}