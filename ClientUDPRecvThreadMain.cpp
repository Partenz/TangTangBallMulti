#include <cstdint>
#include <utility>
#include "ClientUDPRecvThreadMain.h"
#include "EventQueue.h"
#include "EnDePacket.h"
#include "IsNewerSequence.h"

// TODO: 클라이언트 네트워크 초기화 쪽에서 정의하고 초기화.
extern SOCKET clientUdpSocket;
extern sockaddr_in serverUdpAddress;
extern HANDLE hStopEvent;

extern HANDLE hUdpRegisteredEvent;
extern std::queue<NetEvent> clientReceiveQueue;
extern HANDLE hClientReceiveGate;
extern HANDLE hClientReceiveReady;

DWORD WINAPI ClientUdpRecvThreadMain(LPVOID unused)
{
    constexpr BYTE udpRegisteredPacketType = 101;
    constexpr BYTE playersStatePacketType = 111;
    constexpr BYTE playersPreviewPacketType = 121;

    BYTE receiveBuffer[512];

    // 0번은 PLAYERS_STATE, 1~255번은 참가자별 PLAYERS_PREVIEW.
    std::uint32_t lastReceivedSequence[4]{};
    bool hasReceivedSequence[4]{};

    while (true) {
        const DWORD stopEventWaitResult = WaitForSingleObject(hStopEvent, 0);
        if (stopEventWaitResult == WAIT_OBJECT_0)
            return 0;
        if (stopEventWaitResult == WAIT_FAILED)
            return GetLastError();

        sockaddr_in senderAddress{};
        int senderAddressSize = sizeof(senderAddress);
        const int bytesReceived = recvfrom(clientUdpSocket, reinterpret_cast<char*>(receiveBuffer), sizeof(receiveBuffer), 0,
            reinterpret_cast<sockaddr*>(&senderAddress), &senderAddressSize);

        if (bytesReceived == SOCKET_ERROR) {
            const int receiveErrorCode = WSAGetLastError();
            if (receiveErrorCode == WSAETIMEDOUT)
                continue;

            const wchar_t* receiveErrorMessage = L"Winsock 수신 오류입니다.\n";
            switch (receiveErrorCode) {
            case WSAEMSGSIZE:
                receiveErrorMessage = L"패킷이 수신 버퍼보다 커서 버렸습니다.\n";
                break;
            case WSAECONNRESET:
                receiveErrorMessage = L"이전 UDP 전송의 대상 포트에 도달할 수 없습니다.\n";
                break;
            case WSAENOTSOCK:
                receiveErrorMessage = L"잘못된 소켓 핸들입니다.\n";
                break;
            case WSAESHUTDOWN:
                receiveErrorMessage = L"소켓의 수신 기능이 종료되었습니다.\n";
                break;
            case WSANOTINITIALISED:
                receiveErrorMessage = L"Winsock 초기화가 필요합니다.\n";
                break;
            }

            OutputDebugStringW(receiveErrorMessage);

            if (receiveErrorCode == WSAEMSGSIZE or receiveErrorCode == WSAECONNRESET)
                continue;
            return static_cast<DWORD>(receiveErrorCode);
        }

        const std::vector<BYTE> receivedPacket(receiveBuffer, receiveBuffer + bytesReceived);
        BYTE packetType = 0;
        std::vector<BYTE> packetBody;

        if (not DecodePacket(receivedPacket, packetType, packetBody))
            continue;
        if (packetType != udpRegisteredPacketType and
            packetType != playersStatePacketType and
            packetType != playersPreviewPacketType)
            continue;

        const bool sameIpAddress = senderAddress.sin_addr.s_addr == serverUdpAddress.sin_addr.s_addr;
        const bool samePort = senderAddress.sin_port == serverUdpAddress.sin_port;
        if (senderAddress.sin_family != AF_INET or not sameIpAddress or not samePort)
            continue;

        if (packetType == udpRegisteredPacketType) {
            if (not SetEvent(hUdpRegisteredEvent))
                return GetLastError();
            continue;
        }


        // 4바이트로 나뉘어 들어온 UDP 순서 번호를 uint32_t 숫자 하나로 조립
        std::uint32_t receivedSequence = 0;
        for (std::size_t sequenceByteIndex = 2; sequenceByteIndex < 6; ++sequenceByteIndex) {
            receivedSequence = (receivedSequence << 8) | packetBody[sequenceByteIndex];
        }

        const BYTE playerId = packetType == playersPreviewPacketType ? packetBody[6] : 0;
        if (hasReceivedSequence[playerId] and not IsNewerSequence(receivedSequence, lastReceivedSequence[playerId]))
            continue;

        NetEvent receiveEvent{};
        receiveEvent.eventKind = NetEventPacket;
        receiveEvent.playerId = playerId;
        receiveEvent.packetType = packetType;
        receiveEvent.packetData = std::move(packetBody);

        if (not PushEventQueue(clientReceiveQueue, receiveEvent, hClientReceiveGate, hClientReceiveReady))
            return GetLastError();

        lastReceivedSequence[playerId] = receivedSequence;
        hasReceivedSequence[playerId] = true;
    }
}