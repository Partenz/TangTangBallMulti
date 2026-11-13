#include <WinSock2.h>

#include "TcpRecvThread.h"
#include "EnDePacket.h"
#include "EventQueue.h"
#include "NetworkCommon.h"

DWORD RunTcpReceiveLoop(SOCKET socket, HANDLE stopEvent, HANDLE disconnectEvent)
{
    const HANDLE exitEvents[] = { stopEvent, disconnectEvent };
    BYTE chunk[512];
    std::vector<BYTE> pending;

    while (true) {
        const DWORD stop = WaitForMultipleObjects(2, exitEvents, FALSE, 0);
        if (stop == WAIT_OBJECT_0 || stop == WAIT_OBJECT_0 + 1) {
            return ERROR_SUCCESS;
        }
        if (stop == WAIT_FAILED) {
            return GetLastError();
        }

        // 읽을 데이터가 없으면 최대 200ms 뒤에 종료 이벤트를 다시 확인한다.
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

        const int received = recv(socket, reinterpret_cast<char*>(chunk), static_cast<int>(sizeof(chunk)), 0);
        if (received == SOCKET_ERROR) {
            return static_cast<DWORD>(WSAGetLastError());
        }
        if (received == 0) {
            return ERROR_SUCCESS;
        }

        // 이전에 덜 받은 데이터 뒤에 이번에 받은 바이트를 붙인다.
        pending.insert(pending.end(), chunk, chunk + received);

        const auto packets = ExtractTcpPackets(pending);
        const DWORD extractError = GetLastError();
        if (extractError != ERROR_SUCCESS) {
            return extractError;
        }

        for (const auto& packet : packets) {
            NetEvent event{};
            event.eventKind = NetEventPacket;

            if (!DecodePacket(packet, event.packetType, event.packetData)) {
                return ERROR_INVALID_DATA;
            }

            if (!PushEventQueue(clientReceiveQueue, event, hClientReceiveGate, hClientReceiveReady)) {
                return GetLastError();
            }
        }
    }
}