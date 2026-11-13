#include <WinSock2.h>
#include "TcpSendThread.h"

namespace
{
    DWORD CheckStop(const TcpSendConnection& connection)
    {
        const HANDLE events[] = { connection.stopEvent, connection.disconnectEvent };
        const DWORD result = WaitForMultipleObjects(2, events, FALSE, 0);
        if (result == WAIT_TIMEOUT)
            return ERROR_SUCCESS;
        if (result == WAIT_FAILED)
            return GetLastError();
        return ERROR_OPERATION_ABORTED;
    }

    DWORD Finish(const TcpSendConnection& connection, DWORD error)
    {
        if (!SetEvent(connection.disconnectEvent) && error == ERROR_SUCCESS)
            error = GetLastError();
        // 종료 이벤트만으로는 다른 스레드의 blocking recv가 깨어나지 않습니다.
        shutdown(connection.socket, SD_BOTH);
        return error;
    }

    DWORD SendPacket(const TcpSendConnection& connection, const std::vector<BYTE>& packet)
    {
        if (packet.size() < 3 || packet.size() > 65535 ||
            ((static_cast<std::size_t>(packet[0]) << 8) | packet[1]) != packet.size())
            return ERROR_INVALID_DATA;

        std::size_t offset = 0;
        while (offset < packet.size()) {
            const DWORD stop = CheckStop(connection);
            if (stop != ERROR_SUCCESS)
                return stop;

            const int sent = send(connection.socket, reinterpret_cast<const char*>(packet.data() + offset), static_cast<int>(packet.size() - offset), 0);
            if (sent == SOCKET_ERROR)
                return static_cast<DWORD>(WSAGetLastError());
            if (sent == 0)
                return WSAECONNRESET;

            // send가 요청한 길이보다 적게 보내면 남은 바이트부터 이어 보냅니다.
            offset += static_cast<std::size_t>(sent);
        }
        return ERROR_SUCCESS;
    }
}

DWORD RunTcpSendLoop(const TcpSendConnection& connection, std::queue<NetEvent>& queue, HANDLE gate, HANDLE ready)
{
    const HANDLE handles[] = { connection.stopEvent, connection.disconnectEvent, gate, ready };
    if (connection.socket == INVALID_SOCKET)
        return ERROR_INVALID_PARAMETER;
    for (int i = 0; i < 4; ++i) {
        if (handles[i] == nullptr || handles[i] == INVALID_HANDLE_VALUE)
            return ERROR_INVALID_PARAMETER;
        for (int j = 0; j < i; ++j) {
            if (handles[i] == handles[j])
                return ERROR_INVALID_PARAMETER;
        }
    }

    const DWORD timeoutMs = 1000;
    if (setsockopt(connection.socket, SOL_SOCKET, SO_SNDTIMEO, reinterpret_cast<const char*>(&timeoutMs), sizeof(timeoutMs)) == SOCKET_ERROR)
        return Finish(connection, static_cast<DWORD>(WSAGetLastError()));

    const HANDLE waitEvents[] = { connection.stopEvent, connection.disconnectEvent, ready };
    std::vector<NetEvent> packets;
    while (true) {
        const DWORD result = WaitForMultipleObjects(3, waitEvents, FALSE, INFINITE);
        if (result == WAIT_OBJECT_0 || result == WAIT_OBJECT_0 + 1)
            return Finish(connection, ERROR_SUCCESS);
        if (result == WAIT_FAILED)
            return Finish(connection, GetLastError());
        if (result != WAIT_OBJECT_0 + 2)
            return Finish(connection, ERROR_GEN_FAILURE);

        packets.clear();
        if (!DrainEventQueue(queue, packets, gate, ready))
            return Finish(connection, GetLastError());

        for (const NetEvent& packet : packets) {
            const DWORD error = SendPacket(connection, packet.packetData);
            if (error != ERROR_SUCCESS) {
                const DWORD stop = CheckStop(connection);
                return Finish(connection, stop == ERROR_OPERATION_ABORTED ? ERROR_SUCCESS : error);
            }
        }
    }
}
