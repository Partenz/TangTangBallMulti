#include <iostream>
#include <queue>
#include <utility>
#include "ServerUDPRecvThreadMain.h"
#include "ServerConnections.h"
#include "EnDePacket.h"
#include "EventQueue.h"

// TODO: 서버 초기화 코드에서 정의하고 초기화.
SOCKET serverUdpSocket;
HANDLE hStopEvent;

std::queue<NetEvent> serverUdpSendQueue;
HANDLE hServerUdpSendGate;
HANDLE hServerUdpSendReady;

std::queue<NetEvent> gameRequestQueue;
HANDLE hGameRequestGate;
HANDLE hGameRequestReady;

// TODO: 서버에서 한 번 정의하고 hConnectionGate로 두 목록을 보호한다.
// TCP 입장 승인 시, JOIN_RESPONSE를 보내기 전에 발급한 UDP 키를 저장한다.
extern std::unordered_map<std::uint8_t, std::uint64_t> issuedUdpKeys;

// UDP_REGISTER가 확인되면 여기에 송신 주소를 저장한다.
extern std::unordered_map<std::uint8_t, sockaddr_in> registeredUdpAddresses;

DWORD WINAPI ServerUdpRecvThreadMain(LPVOID unused)
{
    constexpr BYTE udpRegisterPacketType = 100;
    constexpr BYTE udpRegisteredPacketType = 101;
    constexpr BYTE moveInputPacketType = 110;
    constexpr BYTE previewStatePacketType = 120;

    const HANDLE connectionGateWaitEvents[] = { hStopEvent, hConnectionGate };
    BYTE receiveBuffer[512];

    while (true) {
        const DWORD stopEventWaitResult = WaitForSingleObject(hStopEvent, 0);
        if (stopEventWaitResult == WAIT_OBJECT_0)
            return 0;
        if (stopEventWaitResult == WAIT_FAILED)
            return GetLastError();

        sockaddr_in senderAddress{};
        int senderAddressSize = sizeof(senderAddress);
        const int bytesReceived = recvfrom(serverUdpSocket, reinterpret_cast<char*>(receiveBuffer), sizeof(receiveBuffer), 0, 
            reinterpret_cast<sockaddr*>(&senderAddress), &senderAddressSize);

        if (bytesReceived == SOCKET_ERROR) {
            const int receiveErrorCode = WSAGetLastError();
            if (receiveErrorCode == WSAETIMEDOUT)
                continue;

            const char* receiveErrorMessage = "Winsock 수신 오류입니다.";
            switch (receiveErrorCode) {
            case WSAEMSGSIZE:
                receiveErrorMessage = "패킷이 수신 버퍼보다 커서 버렸습니다.";
                break;

            case WSAECONNRESET:
                receiveErrorMessage = "이전 UDP 전송의 대상 포트에 도달할 수 없습니다.";
                break;

            case WSAENOTSOCK:
                receiveErrorMessage = "잘못된 소켓 핸들입니다.";
                break;

            case WSAESHUTDOWN:
                receiveErrorMessage = "소켓의 수신 기능이 종료되었습니다.";
                break;

            case WSANOTINITIALISED:
                receiveErrorMessage = "Winsock 초기화가 필요합니다.";
                break;
            }

            std::cout << "UDP 수신 실패 (오류 코드 " << receiveErrorCode << "). " << receiveErrorMessage << '\n';

            if (receiveErrorCode == WSAEMSGSIZE or receiveErrorCode == WSAECONNRESET)
                continue;

            return static_cast<DWORD>(receiveErrorCode);
        }

        const std::vector<BYTE> receivedPacket(receiveBuffer, receiveBuffer + bytesReceived);
        BYTE packetType = 0;
        std::vector<BYTE> packetBody;

        if (not DecodePacket(receivedPacket, packetType, packetBody))
            continue;
        if (packetType != udpRegisterPacketType and
            packetType != moveInputPacketType and
            packetType != previewStatePacketType)
            continue;

        const DWORD connectionGateWaitResult = WaitForMultipleObjects(2, connectionGateWaitEvents, FALSE, INFINITE);
        if (connectionGateWaitResult == WAIT_OBJECT_0)
            return 0;
        if (connectionGateWaitResult == WAIT_FAILED)
            return GetLastError();

        // 이번 패킷의 출발지 IP와 포트로 송신자의 playerId를 찾는다.
        std::uint8_t registeredSenderId = 0;
        for (const auto& [playerId, registeredAddress] : registeredUdpAddresses) {
            const bool sameIpAddress = registeredAddress.sin_addr.s_addr == senderAddress.sin_addr.s_addr;
            const bool samePort = registeredAddress.sin_port == senderAddress.sin_port;

            if (sameIpAddress and samePort) {
                registeredSenderId = playerId;
                break;
            }
        }

        std::uint8_t senderPlayerId = registeredSenderId;
        DWORD registrationError = ERROR_SUCCESS;

        if (packetType == udpRegisterPacketType) {
            senderPlayerId = 0;

            // DecodePacket()이 본문 길이를 확인했다.
            // 첫 바이트는 참가자 번호, 뒤의 8바이트는 UDP 키.
            const std::uint8_t requestedPlayerId = packetBody[0];
            std::uint64_t receivedUdpKey = 0;
            for (std::size_t keyByteIndex = 1; keyByteIndex < packetBody.size(); ++keyByteIndex) {
                receivedUdpKey = (receivedUdpKey << 8) | packetBody[keyByteIndex];
            }

            const auto issuedKeyEntry = issuedUdpKeys.find(requestedPlayerId);
            if (issuedKeyEntry != issuedUdpKeys.end() and issuedKeyEntry->second == receivedUdpKey) {
                if (registeredSenderId == requestedPlayerId) {
                    // 같은 주소에서 재요청하면 등록 확인을 다시 보낸다.
                    senderPlayerId = requestedPlayerId;
                }
                else if (registeredSenderId == 0 and registeredUdpAddresses.find(requestedPlayerId) == registeredUdpAddresses.end()) {
                    try {
                        registeredUdpAddresses.emplace(requestedPlayerId, senderAddress);
                        senderPlayerId = requestedPlayerId;
                    }
                    catch (const std::bad_alloc&) {
                        registrationError = ERROR_NOT_ENOUGH_MEMORY;
                    }
                }
            }
        }

        // 큐에 넣기 전에 연결 목록 Gate를 반납한다.
        if (!SetEvent(hConnectionGate))
            return GetLastError();
        if (registrationError != ERROR_SUCCESS)
            return registrationError;
        if (senderPlayerId == 0)
            continue;

        if (packetType == udpRegisterPacketType) {
            NetEvent registrationReply{};
            registrationReply.playerId = senderPlayerId;
            registrationReply.packetType = udpRegisteredPacketType;
            registrationReply.packetData = EncodePacket(udpRegisteredPacketType, {});

            if (not PushEventQueue(serverUdpSendQueue, registrationReply, hServerUdpSendGate, hServerUdpSendReady))
                return ERROR_GEN_FAILURE;
        }
        else {
            NetEvent gameRequest{};
            gameRequest.eventKind = NetEventPacket;
            gameRequest.playerId = senderPlayerId;
            gameRequest.packetType = packetType;
            gameRequest.packetData = std::move(packetBody);

            if (not PushEventQueue(gameRequestQueue, gameRequest, hGameRequestGate, hGameRequestReady))
                return ERROR_GEN_FAILURE;
        }
    }
}