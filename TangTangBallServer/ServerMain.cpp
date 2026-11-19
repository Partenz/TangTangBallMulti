#include <WinSock2.h>
#include <Windows.h>
#include <filesystem>
#include <fstream>
#include <iostream>

#include "ServerVariable.h"
#include "ServerConnections.h"
#include "InitSyncEvents.h"
#include "CloseSyncEvents.h"
#include "NetworkCommon.h"
#include "SignalStop.h"
#include "ServerAcceptThreadMain.h"
#include "ServerGameThreadMain.h"
#include "ServerUDPRecvThreadMain.h"
#include "ServerUDPSendThreadMain.h"

const char* serverIp = "119.195.162.219";
constexpr unsigned short tcpPort = 9000;
constexpr unsigned short udpPort = 9001;

int main()
{
    // Visual Studio와 실행 파일 직접 실행 모두 같은 기본 맵을 사용한다.
    wchar_t executablePath[MAX_PATH]{};
    const DWORD pathLength = GetModuleFileNameW(nullptr, executablePath, MAX_PATH);
    if (pathLength == 0 || pathLength >= MAX_PATH)
        return 1;

    std::ifstream mapFile(std::filesystem::path(executablePath).parent_path() / L"stage1.txt");
    int objectCount = 0;
    int starCount = 0;
    if (!(mapFile >> objectCount >> starCount >> gameState.spawnX >> gameState.spawnY) ||
        objectCount <= 0 || objectCount > (65535 - 21) / 22 || starCount != 1) {
        std::cerr << "stage1.txt: invalid or missing map header\n";
        return 1;
    }

    // stage1에는 맵 크기 필드가 없어 가장 오른쪽과 아래쪽 객체 경계를 사용한다.
    for (int index = 0; index < objectCount; ++index) {
        ObjectInfo object{};
        int blockType = 0;
        if (!(mapFile >> blockType >> object.left >> object.top >> object.right >> object.bottom) ||
            blockType < 0 || blockType > 8 || object.left < 0 || object.top < 0 ||
            object.left >= object.right || object.top >= object.bottom ||
            object.right > 65535 || object.bottom > 65535) {
            std::cerr << "stage1.txt: invalid object " << index + 1 << '\n';
            return 1;
        }
        object.objectId = index + 1;
        object.blockType = static_cast<BYTE>(blockType);
        gameState.objects.push_back(object);
        if (object.right > gameState.mapWidth)
            gameState.mapWidth = static_cast<uint16_t>(object.right);
        if (object.bottom > gameState.mapHeight)
            gameState.mapHeight = static_cast<uint16_t>(object.bottom);
    }
    gameState.nextObjectId = objectCount + 1;

    WSADATA wsa{};
    int retval = WSAStartup(MAKEWORD(2, 2), &wsa);
    if (retval != 0) {
        std::cerr << "WSAStartup 실패 (" << retval << ")\n";
        return 1;
    }

    if (!InitSyncEvents()) {
        std::cerr << "이벤트 초기화 실패 (" << GetLastError() << ")\n";
        WSACleanup();
        return 1;
    }

    SOCKET listenSocket = INVALID_SOCKET;
    HANDLE acceptThread = nullptr;
    HANDLE gameThread = nullptr;
    HANDLE udpRecvThread = nullptr;
    HANDLE udpSendThread = nullptr;

    // 설정 중 실패하면 아래 공통 정리 코드로 이동한다.
    do {
        // TCP 리스닝 소켓 생성
        listenSocket = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);
        if (listenSocket == INVALID_SOCKET) {
            std::cerr << "TCP socket() 실패 (" << WSAGetLastError() << ")\n";
            break;
        }

        // TCP 주소와 포트 바인딩
        sockaddr_in tcpAddress{};
        tcpAddress.sin_family = AF_INET;
        tcpAddress.sin_addr.s_addr = htonl(INADDR_ANY);
        tcpAddress.sin_port = htons(tcpPort);

        retval = bind(listenSocket, reinterpret_cast<sockaddr*>(&tcpAddress), sizeof(tcpAddress));
        if (retval == SOCKET_ERROR) {
            std::cerr << "TCP bind() 실패 (" << WSAGetLastError() << ")\n";
            break;
        }

        // TCP 접속 대기 시작
        retval = listen(listenSocket, SOMAXCONN);
        if (retval == SOCKET_ERROR) {
            std::cerr << "TCP listen() 실패 (" << WSAGetLastError() << ")\n";
            break;
        }

        // UDP 송수신 소켓 생성
        serverUdpSocket = socket(AF_INET, SOCK_DGRAM, IPPROTO_UDP);
        if (serverUdpSocket == INVALID_SOCKET) {
            std::cerr << "UDP socket() 실패 (" << WSAGetLastError() << ")\n";
            break;
        }

        // UDP 주소와 포트 바인딩
        sockaddr_in udpAddress{};
        udpAddress.sin_family = AF_INET;
        udpAddress.sin_addr.s_addr = htonl(INADDR_ANY);
        udpAddress.sin_port = htons(udpPort);

        retval = bind(serverUdpSocket, reinterpret_cast<sockaddr*>(&udpAddress), sizeof(udpAddress));
        if (retval == SOCKET_ERROR) {
            std::cerr << "UDP bind() 실패 (" << WSAGetLastError() << ")\n";
            break;
        }

        // UDP 송수신 타임아웃 설정
        if (!ApplySocketTimeouts(serverUdpSocket, 1000, 1000)) {
            std::cerr << "UDP 타임아웃 설정 실패 (" << WSAGetLastError() << ")\n";
            break;
        }

        // 여기부터 기존 스레드 생성 코드

        // 게임 처리와 UDP 송수신을 먼저 시작한다.
        gameThread = CreateThread(nullptr, 0, ServerGameThreadMain, nullptr, 0, nullptr);

        if (gameThread == nullptr) {
            std::cerr << "게임 스레드 생성 실패 (" << GetLastError() << ")\n";
            break;
        }

        udpRecvThread = CreateThread(nullptr, 0, ServerUdpRecvThreadMain, nullptr, 0, nullptr);

        if (udpRecvThread == nullptr) {
            std::cerr << "UDP 수신 스레드 생성 실패 (" << GetLastError() << ")\n";
            break;
        }

        udpSendThread = CreateThread(nullptr, 0, ServerUdpSendThreadMain, nullptr, 0, nullptr);

        if (udpSendThread == nullptr) {
            std::cerr << "UDP 송신 스레드 생성 실패 (" << GetLastError() << ")\n";
            break;
        }

        // 접속을 받을 준비가 끝났으므로 accept 스레드를 시작한다.
        acceptThread = CreateThread(nullptr, 0, ServerAcceptThreadMain, &listenSocket, 0, nullptr);

        if (acceptThread == nullptr) {
            std::cerr << "accept 스레드 생성 실패 (" << GetLastError() << ")\n";
            break;
        }

        std::cout << "서버 시작\n"
            << "외부 접속 주소 " << serverIp << '\n'
            << "TCP " << tcpPort << ", UDP " << udpPort << '\n'
            << "종료하려면 Enter를 누르세요.\n";
        std::cin.get();

    } while (false);

    // 새 접속 수락을 끝내고 연결된 참가자에게 종료를 요청한다.
    SetEvent(hStopEvent);
    if (acceptThread != nullptr) {
        WaitForSingleObject(acceptThread, INFINITE);
        CloseHandle(acceptThread);
    }
    SignalStop();

    const HANDLE startedThreads[] = { gameThread, udpRecvThread, udpSendThread };

    for (HANDLE thread : startedThreads) {
        if (thread != nullptr) {
            WaitForSingleObject(thread, INFINITE);
            CloseHandle(thread);
        }
    }

    // 참가자별 TCP 스레드가 끝난 뒤 연결 자원을 회수한다.
    for (const auto& [playerId, connection] : connections) {
        if (connection.sendThread != nullptr)
            WaitForSingleObject(connection.sendThread, INFINITE);
        if (connection.recvThread != nullptr)
            WaitForSingleObject(connection.recvThread, INFINITE);
    }
    CollectFinishedConnections();

    if (listenSocket != INVALID_SOCKET)
        closesocket(listenSocket);
    if (serverUdpSocket != INVALID_SOCKET)
        closesocket(serverUdpSocket);

    CloseSyncEvents();
    WSACleanup();
}