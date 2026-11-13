#include <Winsock2.h>
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

        while (received < size) {
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
	address.sin_addr.s_addr = htonl(INADDR_LOOPBACK);
    

    const SOCKET socketHandle = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);
    if (socketHandle == INVALID_SOCKET) {
        return false;
    }

    constexpr DWORD timeoutMs = 1000;
    if (connect(socketHandle, reinterpret_cast<const sockaddr*>(&address),
        static_cast<int>(sizeof(address))) == SOCKET_ERROR ||
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