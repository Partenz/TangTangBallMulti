#include <iostream>
#include "ServerUDPSendThreadMain.h"
#include "EventQueue.h"
#include "ServerConnections.h"

// TODO: 서버 초기화 코드에서 정의하고 초기화.
extern SOCKET serverUdpSocket;
extern HANDLE hStopEvent;
extern std::queue<NetEvent> serverUdpSendQueue;
extern HANDLE hServerUdpSendGate;
extern HANDLE hServerUdpSendReady;

DWORD WINAPI ServerUdpSendThreadMain(LPVOID unused)
{
    const HANDLE sendQueueWaitEvents[] = { hStopEvent, hServerUdpSendReady };
    const HANDLE connectionGateWaitEvents[] = { hStopEvent, hConnectionGate };
    std::vector<NetEvent> udpEventsToSend;

    while (true) {
        const DWORD sendQueueWaitResult = WaitForMultipleObjects(2, sendQueueWaitEvents, FALSE, INFINITE);
        if (sendQueueWaitResult == WAIT_OBJECT_0)
            return 0;
        if (sendQueueWaitResult == WAIT_FAILED)
            return GetLastError();

        udpEventsToSend.clear();
        if (not DrainEventQueue(serverUdpSendQueue, udpEventsToSend, hServerUdpSendGate, hServerUdpSendReady))
            return GetLastError();

        for (const NetEvent& sendEvent : udpEventsToSend) {
            const DWORD connectionGateWaitResult = WaitForMultipleObjects(2, connectionGateWaitEvents, FALSE, INFINITE);
            if (connectionGateWaitResult == WAIT_OBJECT_0)
                return 0;
            if (connectionGateWaitResult == WAIT_FAILED)
                return GetLastError();

            sockaddr_in destinationAddress{};
            const auto addressEntry = registeredUdpAddresses.find(sendEvent.playerId);
            const bool hasRegisteredAddress = addressEntry != registeredUdpAddresses.end();
            if (hasRegisteredAddress)
                destinationAddress = addressEntry->second;

            if (not SetEvent(hConnectionGate))
                return GetLastError();
            if (not hasRegisteredAddress)
                continue;

            const std::vector<BYTE>& encodedPacket = sendEvent.packetData;
            if (encodedPacket.size() < 3 or encodedPacket.size() > 65507)
                continue;

            const int packetSize = static_cast<int>(encodedPacket.size());
            const int bytesSent = sendto(serverUdpSocket, reinterpret_cast<const char*>(encodedPacket.data()), packetSize, 0,
                reinterpret_cast<const sockaddr*>(&destinationAddress), sizeof(destinationAddress));

            if (bytesSent == SOCKET_ERROR) {
                const int sendErrorCode = WSAGetLastError();
                std::cout << "서버 UDP 전송 실패 (오류 코드 " << sendErrorCode << ")\n";

                if (sendErrorCode == WSAENOTSOCK or
                    sendErrorCode == WSAESHUTDOWN or
                    sendErrorCode == WSANOTINITIALISED)
                    return static_cast<DWORD>(sendErrorCode);

                continue;
            }

            if (bytesSent != packetSize)
                std::cout << "서버 UDP 전송 실패. 패킷 전체를 보내지 못했습니다.\n";
        }
    }
}