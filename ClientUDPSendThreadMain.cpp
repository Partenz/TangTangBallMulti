#include <cstdint>
#include <queue>
#include <vector>
#include <string>
#include "ClientUDPSendThreadMain.h"
#include "EventQueue.h"

DWORD WINAPI ClientUdpSendThreadMain(LPVOID unused)
{
    const HANDLE sendThreadWaitEvents[] = { hStopEvent, hClientUdpSendReady };
    std::vector<NetEvent> dequeuedSendEvents;

    while (true) {
        const DWORD sendQueueWaitResult = WaitForMultipleObjects(2, sendThreadWaitEvents, FALSE, INFINITE);
        if (sendQueueWaitResult == WAIT_OBJECT_0)
            return 0;
        if (sendQueueWaitResult == WAIT_FAILED)
            return GetLastError();

        dequeuedSendEvents.clear();
        if (not DrainEventQueue(clientUdpSendQueue, dequeuedSendEvents, hClientUdpSendGate, hClientUdpSendReady)) {
            return ERROR_GEN_FAILURE;
        }

        for (const NetEvent& sendEvent : dequeuedSendEvents) {
            const DWORD stopEventWaitResult = WaitForSingleObject(hStopEvent, 0);
            if (stopEventWaitResult == WAIT_OBJECT_0)
                return 0;
            if (stopEventWaitResult == WAIT_FAILED)
                return GetLastError();

            const std::vector<BYTE>& outgoingPacket = sendEvent.packetData;
            const int packetSizeBytes = static_cast<int>(outgoingPacket.size());
            const int bytesSent = sendto(clientUdpSocket, reinterpret_cast<const char*>(outgoingPacket.data()), packetSizeBytes, 0, 
                reinterpret_cast<const sockaddr*>(&serverUdpAddress), sizeof(serverUdpAddress));

            if (bytesSent == SOCKET_ERROR) {
                const int sendErrorCode = WSAGetLastError();

                switch (sendErrorCode) {
                case WSAENOTSOCK:
                    OutputDebugStringW(L"UDP 전송 실패 (WSAENOTSOCK). 잘못된 소켓 핸들입니다.\n");
                    break;

                case WSAESHUTDOWN:
                    OutputDebugStringW(L"UDP 전송 실패 (WSAESHUTDOWN). 소켓의 송신 기능이 종료되었습니다.\n");
                    break;

                case WSANOTINITIALISED:
                    OutputDebugStringW(L"UDP 전송 실패 (WSANOTINITIALISED). Winsock 초기화가 필요합니다.\n");
                    break;

                default: {
                    const std::wstring sendErrorMessage = L"UDP 전송 실패. Winsock 오류 코드 " + std::to_wstring(sendErrorCode) + L"\n";
                    OutputDebugStringW(sendErrorMessage.c_str());
                    continue;
                }
                }

                return static_cast<DWORD>(sendErrorCode);
            }

            if (bytesSent != packetSizeBytes)
                OutputDebugStringA("클라이언트 UDP 전송 실패\n");
        }
    }
}