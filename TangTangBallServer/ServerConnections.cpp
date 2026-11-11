#include <Winsock2.h>
#include <Windows.h>
#include <unordered_map>

#include "ServerConnections.h"

#pragma comment(lib, "ws2_32.lib")

// 나중에 서버가 accept()로 받은 참가자별 연결을 등록한다.
std::unordered_map<std::uint8_t, TcpConnection> connections;

// 서버 초기화에서 InitSyncEvents()로 생성한다.
HANDLE hConnectionGate = nullptr;

std::unordered_map<std::uint8_t, std::uint64_t> issuedUdpKeys;
std::unordered_map<std::uint8_t, sockaddr_in> registeredUdpAddresses;


void RequestDisconnect(std::uint8_t playerId)
{
    // 연결 목록을 조회하고 상태를 바꾸기 전에 접근 권한을 얻는다.
    if (WaitForSingleObject(hConnectionGate, INFINITE) != WAIT_OBJECT_0) {
        return;
    }

    auto it = connections.find(playerId);

    // 이미 종료 요청을 받은 연결은 건너뛴다.
    // 송신·수신 스레드가 동시에 요청해도 종료 사건은 한 번만 등록한다.
    if (it == connections.end() ||
        it->second.state != ConnectionState::Active) {
        SetEvent(hConnectionGate);
        return;
    }

    TcpConnection& conn = it->second;
    conn.state = ConnectionState::Stopping;

    // 연결 종료를 시작하면 해당 참가자의 UDP 등록도 해제한다.
    issuedUdpKeys.erase(playerId);
    registeredUdpAddresses.erase(playerId);

    const SOCKET socket = conn.socket;
    const HANDLE disconnectEvent = conn.disconnectEvent;

    // 필요한 값만 복사
    SetEvent(hConnectionGate);

    // 송수신 스레드에 종료 요청, 소켓 종료,
    SetEvent(disconnectEvent);
    shutdown(socket, SD_BOTH);


    if (WaitForSingleObject(hConnectionGate, INFINITE) == WAIT_OBJECT_0) {
        connections.at(playerId).state = ConnectionState::ReadyToCollect;
        SetEvent(hConnectionGate);
    }
}

// 종료 요청 처리가 끝나고 송신·수신 스레드가 모두 종료된 연결을 정리한다. 
void CollectFinishedConnections()
{
    // 정리 가능한 연결을 한 번에 하나씩 찾고 제거.
    for (;;) {
        if (WaitForSingleObject(hConnectionGate, INFINITE) != WAIT_OBJECT_0) {
            return;
        }

        auto it = connections.begin();

        for (; it != connections.end(); ++it) {
            const TcpConnection& conn = it->second;

            if (conn.state == ConnectionState::ReadyToCollect &&
                WaitForSingleObject(conn.sendThread, 0) == WAIT_OBJECT_0 &&
                WaitForSingleObject(conn.recvThread, 0) == WAIT_OBJECT_0) {
                break;
            }
        }

        if (it == connections.end()) {
            SetEvent(hConnectionGate);
            return;
        }

        const TcpConnection finished = it->second;
        connections.erase(it);
        SetEvent(hConnectionGate);

        closesocket(finished.socket);
        CloseHandle(finished.sendThread);
        CloseHandle(finished.recvThread);
        CloseHandle(finished.disconnectEvent);

        if (finished.sendQueueGate != nullptr) {
            CloseHandle(finished.sendQueueGate);
        }
        if (finished.sendQueueReady != nullptr) {
            CloseHandle(finished.sendQueueReady);
        }
    }
}