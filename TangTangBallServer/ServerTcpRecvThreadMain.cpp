#include <vector>

#include "ServerTcpRecvThreadMain.h"
#include "ServerConnections.h"
#include "NetworkCommon.h"
#include "EnDePacket.h"
#include "EventQueue.h"

namespace
{
    DWORD RunTcpReceiveLoop(SOCKET socket, HANDLE disconnectEvent, BYTE playerId)
    {
        const HANDLE exitEvents[] = { hStopEvent, disconnectEvent };
		BYTE chunk[512]; // recv()에 받은 데이터를 임시로 담을 버퍼
        std::vector<BYTE> pending;

        while (true) {
            const DWORD stop = WaitForMultipleObjects(2, exitEvents, FALSE, 0);
            if (stop == WAIT_FAILED) {
                return GetLastError();
            }
            if (stop != WAIT_TIMEOUT) {
                return ERROR_SUCCESS;
            }

            // 수신할 데이터가 없으면 최대 200ms 대기한다.
            fd_set readable;
            FD_ZERO(&readable);
            FD_SET(socket, &readable);

            timeval waitTime{ 0, 200000 };
            const int ready = select(0, &readable, nullptr, nullptr, &waitTime);
            if (ready == SOCKET_ERROR) {
                return static_cast<DWORD>(WSAGetLastError());
            }
            if (ready == 0) {
                continue;
            }

            const int received = recv(socket, reinterpret_cast<char*>(chunk), sizeof(chunk), 0);
            if (received == SOCKET_ERROR) {
                return static_cast<DWORD>(WSAGetLastError());
            }
            if (received == 0) {
                return ERROR_SUCCESS;
            }

            // 덜 받은 데이터 뒤에 이번에 받은 바이트를 붙인다.
            pending.insert(pending.end(), chunk, chunk + received);

            const auto packets = ExtractTcpPackets(pending);
            const DWORD extractError = GetLastError();
            if (extractError != ERROR_SUCCESS) {
                return extractError;
            }

            for (const auto& packet : packets) {
                NetEvent event{};
                event.eventKind = NetEventPacket;
                event.playerId = playerId;

                if (!DecodePacket(packet, event.packetType, event.packetData)) {
                    return ERROR_INVALID_DATA;
                }

                if (!PushEventQueue(gameRequestQueue, event, hGameRequestGate, hGameRequestReady)) {
                    return GetLastError();
                }
            }
        }
    }
}

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