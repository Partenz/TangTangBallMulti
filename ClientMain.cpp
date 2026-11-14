#include <WinSock2.h>
#include <WS2tcpip.h>
#include <Windows.h>
#include <mmsystem.h>
#include <cwchar>
#include <string>
#include <vector>

#include "ClientVariable.h"
#include "SyncEvents.h"
#include "CloseSyncEvents.h"
#include "NetworkCommon.h"
#include "ConnectToServer.h"
#include "RegisterUDP.h"
#include "ClientUDPRecvThreadMain.h"
#include "ClientUDPSendThreadMain.h"
#include "ClientTcpRecvThreadMain.h"
#include "ClientTcpSendThreadMain.h"
#include "EventQueue.h"
#include "ApplyServerEvent.h"
#include "function.h"
#include "resource.h"

#pragma comment(lib, "winmm.lib")

constexpr wchar_t clientWindowClass[] = L"TangTangBallClient";
constexpr UINT_PTR networkTimer = 1;
constexpr UINT_PTR menuTimer = 2;
constexpr UINT menuFrameInterval = 120;
constexpr int startButtonId = 100;
constexpr int connectButtonId = 101;
constexpr int backButtonId = 102;
constexpr int exitButtonId = 103;
constexpr int windowWidth = 800;
constexpr int windowHeight = 500;
constexpr RECT menuPanel{ 178, 100, 622, 448 };
constexpr COLORREF panelColor = RGB(240, 252, 251);

enum class ClientScreen { Start, Connect, Lobby };
enum class ConnectResult : DWORD {
    Success, TcpFailed, UdpSocketFailed, UdpBindFailed, UdpTimeoutFailed,
    ThreadFailed, UdpRegisterFailed
};

struct ClientWindow {
    ClientScreen screen = ClientScreen::Start;
    HWND title = nullptr;
    HWND status = nullptr;
    HWND startButton = nullptr;
    HWND exitButton = nullptr;
    HWND connectButton = nullptr;
    HWND backButton = nullptr;
    HWND addressLabels[3]{};
    HWND addressFields[3]{};
    HWND lobbyPlayer = nullptr;
    HWND lobbyList = nullptr;
    HBITMAP menuFrames[9]{};
    HBITMAP menuButtons[2]{};
    UINT menuFrame = 0;
    HBRUSH backgroundBrush = nullptr;
    HBRUSH panelBrush = nullptr;
    HBRUSH borderBrush = nullptr;
    HFONT titleFont = nullptr;
    HFONT menuButtonFont = nullptr;
    HFONT menuTextFont = nullptr;
    char serverIp[INET_ADDRSTRLEN]{};
    unsigned short tcpPort = 0;
    unsigned short udpPort = 0;
    HANDLE connectThread = nullptr;
    HANDLE udpRecvThread = nullptr;
    HANDLE udpSendThread = nullptr;
    HANDLE tcpRecvThread = nullptr;
    HANDLE tcpSendThread = nullptr;

    ~ClientWindow()
    {
        for (HBITMAP frame : menuFrames)
            DeleteObject(frame);
        for (HBITMAP bitmap : menuButtons)
            DeleteObject(bitmap);
        DeleteObject(backgroundBrush);
        DeleteObject(panelBrush);
        DeleteObject(borderBrush);
        DeleteObject(titleFont);
        DeleteObject(menuButtonFont);
        DeleteObject(menuTextFont);
    }
};

LRESULT CALLBACK ClientWndProc(HWND hWnd, UINT message, WPARAM wParam, LPARAM lParam);

int WINAPI WinMain(HINSTANCE hInstance, HINSTANCE, LPSTR, int nCmdShow)
{
    WSADATA wsa{};
    int retval = WSAStartup(MAKEWORD(2, 2), &wsa);
    if (retval != 0) {
        MessageBoxW(nullptr, L"Winsock을 초기화할 수 없습니다.", L"탱탱볼", MB_ICONERROR);
        return 1;
    }

    WNDCLASSW windowClass{};
    windowClass.hInstance = hInstance;
    windowClass.lpfnWndProc = ClientWndProc;
    windowClass.lpszClassName = clientWindowClass;
    windowClass.hCursor = LoadCursor(nullptr, IDC_ARROW);
    windowClass.hbrBackground = GetSysColorBrush(COLOR_WINDOW);
    if (!RegisterClassW(&windowClass)) {
        WSACleanup();
        return 1;
    }

    ClientWindow client{};
    constexpr DWORD windowStyle = WS_CAPTION | WS_SYSMENU | WS_MINIMIZEBOX | WS_CLIPCHILDREN;
    RECT windowRect{ 0, 0, windowWidth, windowHeight };
    AdjustWindowRect(&windowRect, windowStyle, FALSE);
    HWND hWnd = CreateWindowExW(0, clientWindowClass, L"탱탱볼", windowStyle,
        CW_USEDEFAULT, CW_USEDEFAULT, windowRect.right - windowRect.left,
        windowRect.bottom - windowRect.top, nullptr, nullptr, hInstance, &client);
    if (hWnd == nullptr) {
        WSACleanup();
        return 1;
    }

    ShowWindow(hWnd, nCmdShow);
    UpdateWindow(hWnd);
    SetFocus(client.startButton);

    MSG message{};
    BOOL received = 0;
    while ((received = GetMessageW(&message, nullptr, 0, 0)) > 0) {
        if (message.message == WM_KEYDOWN && message.wParam == VK_RETURN &&
            client.screen == ClientScreen::Start &&
            (message.hwnd == client.startButton || message.hwnd == client.exitButton)) {
            SendMessageW(message.hwnd, BM_CLICK, 0, 0);
            continue;
        }
        if (message.message == WM_KEYDOWN && message.wParam == VK_RETURN &&
            client.screen == ClientScreen::Connect && IsWindowEnabled(client.connectButton)) {
            bool editingAddress = false;
            for (HWND field : client.addressFields) {
                if (message.hwnd == field)
                    editingAddress = true;
            }
            if (editingAddress) {
                SendMessageW(client.connectButton, BM_CLICK, 0, 0);
                continue;
            }
        }
        if (!IsDialogMessageW(hWnd, &message)) {
            TranslateMessage(&message);
            DispatchMessageW(&message);
        }
    }

    if (IsWindow(hWnd))
        DestroyWindow(hWnd);
    WSACleanup();
    if (received == -1)
        return 1;

    // WinMain은 C++의 main과 달리 정상 종료값도 반환해야 한다.
    return static_cast<int>(message.wParam);
}

static DWORD WINAPI ConnectThreadMain(LPVOID argument)
{
    auto& client = *static_cast<ClientWindow*>(argument);

    // TCP 연결과 입장 응답을 먼저 처리한다.
    if (!ConnectToServer(client.serverIp, client.tcpPort))
        return static_cast<DWORD>(ConnectResult::TcpFailed);

    clientUdpSocket = socket(AF_INET, SOCK_DGRAM, IPPROTO_UDP);
    if (clientUdpSocket == INVALID_SOCKET)
        return static_cast<DWORD>(ConnectResult::UdpSocketFailed);

    // 로컬 포트 0은 운영체제가 비어 있는 UDP 포트를 배정한다는 뜻이다.
    sockaddr_in localUdpAddress{};
    localUdpAddress.sin_family = AF_INET;
    localUdpAddress.sin_addr.s_addr = htonl(INADDR_ANY);
    int retval = bind(clientUdpSocket, reinterpret_cast<sockaddr*>(&localUdpAddress),
        sizeof(localUdpAddress));
    if (retval == SOCKET_ERROR)
        return static_cast<DWORD>(ConnectResult::UdpBindFailed);

    if (!ApplySocketTimeouts(clientUdpSocket, 1000, 1000))
        return static_cast<DWORD>(ConnectResult::UdpTimeoutFailed);

    client.udpRecvThread = CreateThread(nullptr, 0, ClientUdpRecvThreadMain, nullptr, 0, nullptr);
    if (client.udpRecvThread == nullptr)
        return static_cast<DWORD>(ConnectResult::ThreadFailed);

    client.udpSendThread = CreateThread(nullptr, 0, ClientUdpSendThreadMain, nullptr, 0, nullptr);
    if (client.udpSendThread == nullptr)
        return static_cast<DWORD>(ConnectResult::ThreadFailed);

    client.tcpRecvThread = CreateThread(nullptr, 0, ClientTcpRecvThreadMain, nullptr, 0, nullptr);
    if (client.tcpRecvThread == nullptr)
        return static_cast<DWORD>(ConnectResult::ThreadFailed);

    client.tcpSendThread = CreateThread(nullptr, 0, ClientTcpSendThreadMain, nullptr, 0, nullptr);
    if (client.tcpSendThread == nullptr)
        return static_cast<DWORD>(ConnectResult::ThreadFailed);

    // 입장 승인에서 받은 참가자 ID와 키로 UDP를 등록한다.
    if (!RegisterUdp(clientPlayerId, clientUdpKey))
        return static_cast<DWORD>(ConnectResult::UdpRegisterFailed);

    return static_cast<DWORD>(ConnectResult::Success);
}

static void CloseClientConnection(ClientWindow& client)
{
    if (hStopEvent == nullptr)
        return;

    SetEvent(hStopEvent);
    if (client.connectThread != nullptr) {
        WaitForSingleObject(client.connectThread, INFINITE);
        CloseHandle(client.connectThread);
        client.connectThread = nullptr;
    }

    if (clientTcpSocket != INVALID_SOCKET)
        shutdown(clientTcpSocket, SD_BOTH);

    HANDLE* startedThreads[] = {
        &client.udpRecvThread, &client.udpSendThread, &client.tcpRecvThread, &client.tcpSendThread
    };
    for (HANDLE* thread : startedThreads) {
        if (*thread != nullptr) {
            WaitForSingleObject(*thread, INFINITE);
            CloseHandle(*thread);
            *thread = nullptr;
        }
    }

    if (clientUdpSocket != INVALID_SOCKET)
        closesocket(clientUdpSocket);
    if (clientTcpSocket != INVALID_SOCKET)
        closesocket(clientTcpSocket);

    CloseSyncEvents();

    // 다시 접속할 때 이전 연결의 상태와 패킷을 가져가지 않는다.
    clientUdpSocket = INVALID_SOCKET;
    clientTcpSocket = INVALID_SOCKET;
    clientTcpConnection = {};
    clientPlayerId = 0;
    clientUdpKey = 0;
    clientUdpSendSequence = 0;
    clientGameState = {};
    clientTcpSendQueue = {};
    clientUdpSendQueue = {};
    clientReceiveQueue = {};
}

static void ShowScreen(ClientWindow& client, ClientScreen screen, const wchar_t* status)
{
    client.screen = screen;
    const bool showConnect = screen == ClientScreen::Connect;
    const bool showLobby = screen == ClientScreen::Lobby;

    const wchar_t* title = L"Tang Tang Ball";
    if (showConnect)
        title = L"서버 접속";
    if (showLobby)
        title = L"로비";
    SetWindowTextW(client.title, title);
    SetWindowTextW(client.status, status);
    ShowWindow(client.status, screen == ClientScreen::Start ? SW_HIDE : SW_SHOW);

    ShowWindow(client.startButton, screen == ClientScreen::Start ? SW_SHOW : SW_HIDE);
    ShowWindow(client.exitButton, screen == ClientScreen::Start ? SW_SHOW : SW_HIDE);
    ShowWindow(client.connectButton, showConnect ? SW_SHOW : SW_HIDE);
    ShowWindow(client.backButton, screen == ClientScreen::Start ? SW_HIDE : SW_SHOW);
    SetWindowTextW(client.backButton, showLobby ? L"나가기" : L"뒤로");
    EnableWindow(client.connectButton, TRUE);

    for (HWND label : client.addressLabels)
        ShowWindow(label, showConnect ? SW_SHOW : SW_HIDE);
    for (HWND field : client.addressFields) {
        EnableWindow(field, TRUE);
        ShowWindow(field, showConnect ? SW_SHOW : SW_HIDE);
    }

    ShowWindow(client.lobbyPlayer, showLobby ? SW_SHOW : SW_HIDE);
    ShowWindow(client.lobbyList, showLobby ? SW_SHOW : SW_HIDE);

    InvalidateRect(GetParent(client.title), nullptr, FALSE);
    if (showConnect)
        SetFocus(client.addressFields[0]);
    else
        SetFocus(showLobby ? client.backButton : client.startButton);
}

static void StartConnection(ClientWindow& client)
{
    if (client.connectThread != nullptr)
        return;

    wchar_t address[INET_ADDRSTRLEN]{};
    GetWindowTextW(client.addressFields[0], address, ARRAYSIZE(address));
    IN_ADDR serverAddress{};
    if (InetPtonW(AF_INET, address, &serverAddress) != 1) {
        SetWindowTextW(client.status, L"서버의 IPv4 주소를 입력하세요.");
        SetFocus(client.addressFields[0]);
        return;
    }
    if (inet_ntop(AF_INET, &serverAddress, client.serverIp, sizeof(client.serverIp)) == nullptr) {
        SetWindowTextW(client.status, L"서버 주소를 읽을 수 없습니다.");
        return;
    }

    unsigned short* ports[] = { &client.tcpPort, &client.udpPort };
    for (int i = 0; i < 2; ++i) {
        wchar_t portText[16]{};
        GetWindowTextW(client.addressFields[i + 1], portText, ARRAYSIZE(portText));
        wchar_t* end = nullptr;
        const unsigned long port = std::wcstoul(portText, &end, 10);
        if (end == portText || *end != L'\0' || port == 0 || port > 65535) {
            SetWindowTextW(client.status, i == 0 ?
                L"TCP 포트는 1~65535 사이여야 합니다." : L"UDP 포트는 1~65535 사이여야 합니다.");
            SetFocus(client.addressFields[i + 1]);
            return;
        }
        *ports[i] = static_cast<unsigned short>(port);
    }

    if (!InitSyncEvents()) {
        SetWindowTextW(client.status, L"네트워크 이벤트 초기화에 실패했습니다.");
        return;
    }

    serverUdpAddress.sin_family = AF_INET;
    serverUdpAddress.sin_addr = serverAddress;
    serverUdpAddress.sin_port = htons(client.udpPort);

    client.connectThread = CreateThread(nullptr, 0, ConnectThreadMain, &client, 0, nullptr);
    if (client.connectThread == nullptr) {
        CloseClientConnection(client);
        SetWindowTextW(client.status, L"접속 스레드를 시작할 수 없습니다.");
        return;
    }

    for (HWND field : client.addressFields)
        EnableWindow(field, FALSE);
    EnableWindow(client.connectButton, FALSE);
    SetWindowTextW(client.backButton, L"취소");
    SetWindowTextW(client.status, L"서버에 접속하는 중입니다...");
    SetFocus(client.backButton);
}

static void UpdateLobby(ClientWindow& client)
{
    const std::wstring heading = L"내 참가자 번호 " + std::to_wstring(clientPlayerId) +
        L"   |   접속 인원 " + std::to_wstring(clientGameState.players.size()) + L"명";
    SetWindowTextW(client.lobbyPlayer, heading.c_str());

    SendMessageW(client.lobbyList, LB_RESETCONTENT, 0, 0);
    const wchar_t* colors[] = { L"빨강", L"초록", L"파랑", L"청록", L"자홍", L"노랑" };
    for (const auto& entry : clientGameState.players) {
        const PlayerInfo& info = entry.second.info;
        const std::wstring row = L"참가자 " + std::to_wstring(info.playerId) +
            (info.playerId == clientPlayerId ? L" (나)" : L"") +
            L"    |    " + colors[info.color] +
            (info.ready ? L"    |    준비 완료" : L"    |    준비 대기");
        SendMessageW(client.lobbyList, LB_ADDSTRING, 0, reinterpret_cast<LPARAM>(row.c_str()));
    }
}

static void UpdateNetwork(ClientWindow& client)
{
    if (client.connectThread != nullptr) {
        if (WaitForSingleObject(client.connectThread, 0) != WAIT_OBJECT_0)
            return;

        DWORD result = static_cast<DWORD>(ConnectResult::ThreadFailed);
        GetExitCodeThread(client.connectThread, &result);
        CloseHandle(client.connectThread);
        client.connectThread = nullptr;

        if (result != static_cast<DWORD>(ConnectResult::Success)) {
            const wchar_t* status = L"네트워크를 시작할 수 없습니다. 다시 시도하세요.";
            if (result == static_cast<DWORD>(ConnectResult::TcpFailed))
                status = L"서버에 연결하거나 TCP 입장 승인을 받지 못했습니다.";
            else if (result == static_cast<DWORD>(ConnectResult::UdpSocketFailed))
                status = L"UDP 소켓을 만들 수 없습니다.";
            else if (result == static_cast<DWORD>(ConnectResult::UdpBindFailed))
                status = L"로컬 UDP 포트를 열 수 없습니다.";
            else if (result == static_cast<DWORD>(ConnectResult::UdpTimeoutFailed))
                status = L"UDP 타임아웃을 설정할 수 없습니다.";
            else if (result == static_cast<DWORD>(ConnectResult::UdpRegisterFailed))
                status = L"서버의 UDP 등록 확인을 받지 못했습니다.";
            CloseClientConnection(client);
            ShowScreen(client, ClientScreen::Connect, status);
            return;
        }

        ShowScreen(client, ClientScreen::Lobby, L"참가자 목록을 기다리는 중입니다.");
        UpdateLobby(client);
    }

    if (client.screen != ClientScreen::Lobby)
        return;

    const HANDLE networkThreads[] = {
        client.tcpRecvThread, client.tcpSendThread, client.udpRecvThread, client.udpSendThread
    };
    if (WaitForMultipleObjects(ARRAYSIZE(networkThreads), networkThreads, FALSE, 0) != WAIT_TIMEOUT) {
        CloseClientConnection(client);
        ShowScreen(client, ClientScreen::Connect, L"서버와 연결이 끊어졌습니다.");
        return;
    }

    if (WaitForSingleObject(hClientReceiveReady, 0) != WAIT_OBJECT_0)
        return;

    std::vector<NetEvent> events;
    if (!DrainEventQueue(clientReceiveQueue, events, hClientReceiveGate, hClientReceiveReady)) {
        CloseClientConnection(client);
        ShowScreen(client, ClientScreen::Connect, L"서버 응답을 처리할 수 없습니다.");
        return;
    }

    bool lobbyChanged = false;
    for (const NetEvent& event : events) {
        ApplyServerEvent(event);
        if (event.packetType == 2 || event.packetType == 8)
            lobbyChanged = true;
    }
    if (lobbyChanged) {
        UpdateLobby(client);
        SetWindowTextW(client.status, L"서버에 연결되었습니다.");
    }
}

static bool LoadMenuResources(ClientWindow& client, HINSTANCE instance)
{
    if (!LoadMenuBitmaps(instance, client.menuFrames, client.menuButtons))
        return false;

    client.backgroundBrush = CreateSolidBrush(RGB(140, 238, 238));
    client.panelBrush = CreateSolidBrush(panelColor);
    client.borderBrush = CreateSolidBrush(RGB(95, 175, 174));
    client.titleFont = CreateFontW(-48, 0, 0, 0, FW_BOLD, FALSE, FALSE, FALSE, DEFAULT_CHARSET,
        OUT_DEFAULT_PRECIS, CLIP_DEFAULT_PRECIS, CLEARTYPE_QUALITY, DEFAULT_PITCH, L"맑은 고딕");
    client.menuButtonFont = CreateFontW(-22, 0, 0, 0, FW_BOLD, FALSE, FALSE, FALSE, DEFAULT_CHARSET,
        OUT_DEFAULT_PRECIS, CLIP_DEFAULT_PRECIS, CLEARTYPE_QUALITY, DEFAULT_PITCH, L"맑은 고딕");
    client.menuTextFont = CreateFontW(-18, 0, 0, 0, FW_NORMAL, FALSE, FALSE, FALSE, DEFAULT_CHARSET,
        OUT_DEFAULT_PRECIS, CLIP_DEFAULT_PRECIS, CLEARTYPE_QUALITY, DEFAULT_PITCH, L"맑은 고딕");
    return client.backgroundBrush && client.panelBrush && client.borderBrush &&
        client.titleFont && client.menuButtonFont && client.menuTextFont;
}

static void DrawMenuButton(HWND hWnd, const ClientWindow& client, const DRAWITEMSTRUCT& item)
{
    RECT buttonRect = item.rcItem;
    const bool pressed = (item.itemState & ODS_SELECTED) != 0;
    HDC bitmapDC = CreateCompatibleDC(item.hDC);
    if (bitmapDC == nullptr)
        return;

    // 시작 버튼의 투명한 부분에도 현재 애니메이션 프레임을 그린다.
    if (client.screen == ClientScreen::Start) {
        POINT buttonOrigin{};
        MapWindowPoints(item.hwndItem, hWnd, &buttonOrigin, 1);
        RECT clientRect{};
        GetClientRect(hWnd, &clientRect);
        OffsetRect(&clientRect, -buttonOrigin.x, -buttonOrigin.y);
        DrawGameStart(item.hDC, bitmapDC, clientRect, client.menuFrames[client.menuFrame]);
    } else {
        FillRect(item.hDC, &buttonRect, client.panelBrush);
    }

    DrawMenuButtonBitmap(item.hDC, bitmapDC, buttonRect, client.menuButtons[pressed ? 1 : 0]);
    DeleteDC(bitmapDC);

    wchar_t label[32]{};
    GetWindowTextW(item.hwndItem, label, ARRAYSIZE(label));
    const int savedDC = SaveDC(item.hDC);
    SelectObject(item.hDC, client.menuButtonFont);
    SetBkMode(item.hDC, TRANSPARENT);
    SetTextColor(item.hDC, (item.itemState & ODS_DISABLED) ? RGB(105, 105, 105) : RGB(0, 0, 200));
    RECT textRect = buttonRect;
    if (pressed)
        OffsetRect(&textRect, 1, 1);
    DrawTextW(item.hDC, label, -1, &textRect, DT_CENTER | DT_VCENTER | DT_SINGLELINE);
    if ((item.itemState & ODS_FOCUS) && !(item.itemState & ODS_NOFOCUSRECT)) {
        InflateRect(&buttonRect, -7, -6);
        DrawFocusRect(item.hDC, &buttonRect);
    }
    if (savedDC != 0)
        RestoreDC(item.hDC, savedDC);
}

LRESULT CALLBACK ClientWndProc(HWND hWnd, UINT message, WPARAM wParam, LPARAM lParam)
{
    if (message == WM_NCCREATE) {
        const auto* create = reinterpret_cast<const CREATESTRUCTW*>(lParam);
        SetWindowLongPtrW(hWnd, GWLP_USERDATA, reinterpret_cast<LONG_PTR>(create->lpCreateParams));
    }

    auto* client = reinterpret_cast<ClientWindow*>(GetWindowLongPtrW(hWnd, GWLP_USERDATA));
    if (client == nullptr)
        return DefWindowProcW(hWnd, message, wParam, lParam);

    switch (message) {
    case WM_CREATE: {
        const auto* create = reinterpret_cast<const CREATESTRUCTW*>(lParam);
        if (!LoadMenuResources(*client, create->hInstance)) {
            MessageBoxW(hWnd, L"시작 화면을 불러오지 못했습니다.", L"탱탱볼", MB_ICONERROR);
            return -1;
        }

        struct Control {
            HWND* handle;
            const wchar_t* kind;
            const wchar_t* text;
            DWORD style;
            int x, y, width, height;
            int id;
        };
        const Control controls[] = {
            { &client->title, L"STATIC", L"", WS_VISIBLE | SS_CENTER,
                70, 26, 660, 62, 0 },
            { &client->status, L"STATIC", L"", SS_CENTER, 196, 345, 408, 34, 0 },
            { &client->startButton, L"BUTTON", L"START", WS_TABSTOP | BS_OWNERDRAW,
                326, 175, 148, 52, startButtonId },
            { &client->exitButton, L"BUTTON", L"EXIT", WS_TABSTOP | BS_OWNERDRAW,
                326, 267, 148, 52, exitButtonId },
            { &client->addressLabels[0], L"STATIC", L"서버 IPv4 주소", 0, 209, 144, 145, 27, 0 },
            { &client->addressFields[0], L"EDIT", L"", WS_BORDER | WS_TABSTOP | ES_AUTOHSCROLL,
                369, 141, 218, 32, 0 },
            { &client->addressLabels[1], L"STATIC", L"TCP 포트", 0, 209, 206, 145, 27, 0 },
            { &client->addressFields[1], L"EDIT", L"9000", WS_BORDER | WS_TABSTOP | ES_NUMBER,
                369, 203, 218, 32, 0 },
            { &client->addressLabels[2], L"STATIC", L"UDP 포트", 0, 209, 268, 145, 27, 0 },
            { &client->addressFields[2], L"EDIT", L"9001", WS_BORDER | WS_TABSTOP | ES_NUMBER,
                369, 265, 218, 32, 0 },
            { &client->connectButton, L"BUTTON", L"접속", WS_TABSTOP | BS_OWNERDRAW,
                459, 385, 128, 42, connectButtonId },
            { &client->backButton, L"BUTTON", L"뒤로", WS_TABSTOP | BS_OWNERDRAW,
                209, 385, 128, 42, backButtonId },
            { &client->lobbyPlayer, L"STATIC", L"", 0, 209, 129, 378, 28, 0 },
            { &client->lobbyList, L"LISTBOX", L"", WS_BORDER | WS_VSCROLL | LBS_SORT | LBS_NOSEL,
                209, 168, 378, 158, 0 }
        };

        for (const Control& control : controls) {
            *control.handle = CreateWindowExW(0, control.kind, control.text, WS_CHILD | control.style,
                control.x, control.y, control.width, control.height, hWnd,
                reinterpret_cast<HMENU>(static_cast<INT_PTR>(control.id)), create->hInstance, nullptr);
            if (*control.handle == nullptr)
                return -1;
            const HFONT font = control.handle == &client->title ? client->titleFont : client->menuTextFont;
            SendMessageW(*control.handle, WM_SETFONT, reinterpret_cast<WPARAM>(font), FALSE);
        }

        SendMessageW(client->addressFields[0], EM_SETLIMITTEXT, 15, 0);
        SendMessageW(client->addressFields[1], EM_SETLIMITTEXT, 5, 0);
        SendMessageW(client->addressFields[2], EM_SETLIMITTEXT, 5, 0);
        if (SetTimer(hWnd, networkTimer, 50, nullptr) == 0 ||
            SetTimer(hWnd, menuTimer, menuFrameInterval, nullptr) == 0)
            return -1;

        ShowScreen(*client, ClientScreen::Start, L"");
        return 0;
    }
    case WM_PAINT: {
        PAINTSTRUCT paint{};
        HDC target = BeginPaint(hWnd, &paint);
        HDC bitmapDC = CreateCompatibleDC(target);
        if (bitmapDC != nullptr) {
            RECT clientRect{};
            GetClientRect(hWnd, &clientRect);
            DrawGameStart(target, bitmapDC, clientRect, client->menuFrames[client->menuFrame]);
            DeleteDC(bitmapDC);
        } else {
            FillRect(target, &paint.rcPaint, client->backgroundBrush);
        }
        if (client->screen != ClientScreen::Start) {
            FillRect(target, &menuPanel, client->panelBrush);
            FrameRect(target, &menuPanel, client->borderBrush);
        }
        EndPaint(hWnd, &paint);
        return 0;
    }
    case WM_ERASEBKGND:
        return 1;
    case WM_DRAWITEM: {
        const auto& item = *reinterpret_cast<const DRAWITEMSTRUCT*>(lParam);
        if (item.CtlType == ODT_BUTTON) {
            DrawMenuButton(hWnd, *client, item);
            return TRUE;
        }
        break;
    }
    case WM_CTLCOLORSTATIC: {
        const HWND control = reinterpret_cast<HWND>(lParam);
        if (control == client->title) {
            SetTextColor(reinterpret_cast<HDC>(wParam), RGB(0, 0, 0));
            SetBkMode(reinterpret_cast<HDC>(wParam), TRANSPARENT);
            return reinterpret_cast<LRESULT>(client->backgroundBrush);
        }
        SetTextColor(reinterpret_cast<HDC>(wParam), RGB(23, 61, 61));
        SetBkMode(reinterpret_cast<HDC>(wParam), TRANSPARENT);
        return reinterpret_cast<LRESULT>(client->panelBrush);
    }
    case WM_COMMAND:
        if (lParam != 0 && HIWORD(wParam) == BN_CLICKED) {
            PlaySoundW(MAKEINTRESOURCEW(IDW_BUTTON),
                reinterpret_cast<HINSTANCE>(GetWindowLongPtrW(hWnd, GWLP_HINSTANCE)),
                SND_ASYNC | SND_RESOURCE | SND_NODEFAULT);
        }
        switch (LOWORD(wParam)) {
        case startButtonId:
            ShowScreen(*client, ClientScreen::Connect, L"");
            return 0;
        case exitButtonId:
            DestroyWindow(hWnd);
            return 0;
        case connectButtonId:
        case IDOK:
            if (client->screen == ClientScreen::Start)
                ShowScreen(*client, ClientScreen::Connect, L"");
            else if (client->screen == ClientScreen::Connect)
                StartConnection(*client);
            return 0;
        case backButtonId:
        case IDCANCEL:
            CloseClientConnection(*client);
            ShowScreen(*client, ClientScreen::Start, L"");
            return 0;
        }
        break;
    case WM_TIMER:
        if (wParam == networkTimer) {
            UpdateNetwork(*client);
            return 0;
        }
        if (wParam == menuTimer) {
            client->menuFrame = (client->menuFrame + 1) % ARRAYSIZE(client->menuFrames);
            InvalidateRect(hWnd, nullptr, FALSE);
            if (client->screen == ClientScreen::Start) {
                InvalidateRect(client->startButton, nullptr, FALSE);
                InvalidateRect(client->exitButton, nullptr, FALSE);
            }
            return 0;
        }
        break;
    case WM_DESTROY:
        KillTimer(hWnd, networkTimer);
        KillTimer(hWnd, menuTimer);
        PlaySoundW(nullptr, nullptr, 0);
        CloseClientConnection(*client);
        PostQuitMessage(0);
        return 0;
    }

    return DefWindowProcW(hWnd, message, wParam, lParam);
}
