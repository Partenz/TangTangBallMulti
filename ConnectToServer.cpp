#include <Winsock2.h>
#include <WS2tcpip.h>
#include <vector>

#include "ConnectToServer.h"
#include "NetworkCommon.h"
#include "EnDePacket.h"

// 소켓에서 수신하는 함수
namespace 
{
    bool ReceiveExact(SOCKET socket, BYTE* data, int size)
    {
        int received = 0;
        const ULONGLONG deadline = GetTickCount64() + 3000;

        while (received < size) {
            if (WaitForSingleObject(hStopEvent, 0) == WAIT_OBJECT_0 ||
                GetTickCount64() >= deadline)
                return false;

            const int count = recv(socket, reinterpret_cast<char*>(data + received),size - received, 0);

            // 0은 상대가 연결을 닫은 경우, SOCKET_ERROR는 수신 실패.
            if (count <= 0) {
                return false;
            }

            received += count;
        }

        return true;
    }
}

bool ConnectToServer(const char* serverIp, std::uint16_t tcpPort)
{
    if (serverIp == nullptr || tcpPort == 0 ||
        clientTcpSocket != INVALID_SOCKET) {
        return false;
    }

    sockaddr_in address{};
    address.sin_family = AF_INET;
    address.sin_port = htons(tcpPort);
    if (inet_pton(AF_INET, serverIp, &address.sin_addr) != 1)
        return false;
    

    const SOCKET socketHandle = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);
    if (socketHandle == INVALID_SOCKET) {
        return false;
    }

    // 접속을 기다리는 동안에도 창을 닫거나 접속을 취소할 수 있게 한다.
    u_long nonBlocking = 1;
    if (ioctlsocket(socketHandle, FIONBIO, &nonBlocking) == SOCKET_ERROR) {
        closesocket(socketHandle);
        return false;
    }

    bool connected = connect(socketHandle, reinterpret_cast<const sockaddr*>(&address),
        sizeof(address)) == 0;
    if (!connected && WSAGetLastError() == WSAEWOULDBLOCK) {
        const ULONGLONG deadline = GetTickCount64() + 5000;
        while (GetTickCount64() < deadline &&
            WaitForSingleObject(hStopEvent, 0) != WAIT_OBJECT_0) {
            fd_set writable;
            fd_set failed;
            FD_ZERO(&writable);
            FD_ZERO(&failed);
            FD_SET(socketHandle, &writable);
            FD_SET(socketHandle, &failed);

            timeval waitTime{ 0, 100000 };
            const int ready = select(0, nullptr, &writable, &failed, &waitTime);
            if (ready != 0) {
                connected = ready > 0 && FD_ISSET(socketHandle, &writable);
                break;
            }
        }
    }

    nonBlocking = 0;
    constexpr DWORD timeoutMs = 1000;
    if (!connected || WaitForSingleObject(hStopEvent, 0) == WAIT_OBJECT_0 ||
        ioctlsocket(socketHandle, FIONBIO, &nonBlocking) == SOCKET_ERROR ||
        !ApplySocketTimeouts(socketHandle, timeoutMs, timeoutMs)) {
        closesocket(socketHandle);
        return false;
    }

	// 서버로부터 참가 승인 패킷
    constexpr BYTE joinResponseType = 1;
    constexpr int headerSize = 3;
    constexpr int bodySize = 10;
    constexpr int packetSize = headerSize + bodySize;

    std::vector<BYTE> packet(packetSize);


    BYTE packetType = 0;
    std::vector<BYTE> body;

    if (!ReceiveExact(socketHandle, packet.data(), packetSize) ||
        !DecodePacket(packet, packetType, body) ||
        packetType != joinResponseType ||
        body[0] != 1 || body[1] == 0) {
        closesocket(socketHandle);
        return false;
    }

    // 본문 2~9번 바이트를 합쳐 64비트 UDP 키 복원
    std::uint64_t udpKey = 0;
    for (int i = 2; i < bodySize; ++i) {
        udpKey = (udpKey << 8) | body[i];
    }

    clientPlayerId = body[1];
    clientUdpKey = udpKey;
    clientTcpSocket = socketHandle;
    clientTcpConnection.socket = socketHandle;
    return true;
}
