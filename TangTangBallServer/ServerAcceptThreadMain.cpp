#include "ServerAcceptThreadMain.h"
#include <new>
#include "ServerConnections.h"
#include "ServerTcpRecvThreadMain.h"
#include "NetworkCommon.h"
#include "EventQueue.h"
#include "ServerVariable.h"


DWORD WINAPI ServerAcceptThreadMain(LPVOID listenSocketPtr)
{
	if (listenSocketPtr == NULL) return ERROR_INVALID_PARAMETER;

	// listen 소켓을 가져오기
	const SOCKET listenSocket = *static_cast<const SOCKET*>(listenSocketPtr);
	constexpr DWORD timeoutMS = 1000;

	while (true)
	{
		const DWORD stopResult = WaitForSingleObject(hStopEvent, 0);
		if (stopResult == WAIT_OBJECT_0) return 0;
		if (stopResult != WAIT_TIMEOUT) return GetLastError();

		CollectFinishedConnections();

		// 접속 요청이나 종료 신호를 다시 확인할 때까지 기다린다.
		fd_set readable;
		FD_ZERO(&readable);
		FD_SET(listenSocket, &readable);

		timeval waitTime{ 0, 200000 };
		const int ready = select(0, &readable, nullptr, nullptr, &waitTime);

		if (ready == SOCKET_ERROR) return static_cast<DWORD>(WSAGetLastError());
		if (ready == 0) continue;

		// accept()로 연결 수락
		const SOCKET clientSocket = accept(listenSocket, nullptr, nullptr);
		if (clientSocket == INVALID_SOCKET) {
			const int err = WSAGetLastError();
			if (err == WSAEWOULDBLOCK) continue; // 타임아웃
			return static_cast<DWORD>(err);
		}

		// 타임아웃 적용하기
		if (!ApplySocketTimeouts(clientSocket, timeoutMS, timeoutMS)) {
			closesocket(clientSocket);
			continue;
		}

		TcpConnection connection{};
		connection.socket = clientSocket;
		try {
			connection.sendContext = std::make_shared<ServerTcpSendContext>();
		}
		catch (const std::bad_alloc&) {
			CloseTcpConnection(connection);
			return ERROR_NOT_ENOUGH_MEMORY;
		}

		// 참가자별 종료 이벤트와 TCP 송신 큐 이벤트를 준비한다.
		auto& sendContext = *connection.sendContext;
		connection.disconnectEvent = CreateEvent(nullptr, TRUE, FALSE, nullptr);
		if (connection.disconnectEvent == nullptr) {
			const DWORD error = GetLastError();
			CloseTcpConnection(connection);
			return error;
		}

		sendContext.socket = clientSocket;
		sendContext.stopEvent = hStopEvent;
		sendContext.disconnectEvent = connection.disconnectEvent;
		sendContext.sendQueueGate = CreateEvent(nullptr, FALSE, TRUE, nullptr);
		if (sendContext.sendQueueGate == nullptr) {
			const DWORD error = GetLastError();
			CloseTcpConnection(connection);
			return error;
		}

		sendContext.sendQueueReady = CreateEvent(nullptr, TRUE, FALSE, nullptr);
		if (sendContext.sendQueueReady == nullptr) {
			const DWORD error = GetLastError();
			CloseTcpConnection(connection);
			return error;
		}

		if (WaitForSingleObject(hConnectionGate, INFINITE) != WAIT_OBJECT_0) {
			const DWORD error = GetLastError();
			CloseTcpConnection(connection);
			return error;
		}

		// 목록을 보호한 상태에서 사용 중이지 않은 참가자 번호를 찾는다.
		std::uint8_t playerId = 0;
		for (int id = 1; id <= 255; ++id) {
			const auto candidate = static_cast<std::uint8_t>(id);
			if (connections.find(candidate) == connections.end()) {
				playerId = candidate;
				break;
			}
		}

		if (playerId == 0) {
			SetEvent(hConnectionGate);
			CloseTcpConnection(connection);
			continue;
		}

		connection.playerId = playerId;
		TcpConnection* registeredConnection = nullptr;
		try {
			registeredConnection = &connections.emplace(playerId, connection).first->second;
		}
		catch (const std::bad_alloc&) {
			SetEvent(hConnectionGate);
			CloseTcpConnection(connection);
			return ERROR_NOT_ENOUGH_MEMORY;
		}

		// 연결 목록에 저장된 ID를 두 스레드에 전달한다.
		TcpConnection& registered = *registeredConnection;
		registered.sendThread = CreateThread(nullptr, 0, ServerTcpSendThreadMain, &registered.playerId, 0, nullptr);
		if (registered.sendThread != nullptr)
			registered.recvThread = CreateThread(nullptr, 0, ServerTcpRecvThreadMain, &registered.playerId, 0, nullptr);

		// TCP 스레드가 Gate를 얻기 전에 새 접속 사건을 큐에 넣는다.
		NetEvent event{};
		event.eventKind = NetEventConnected;
		event.playerId = playerId;

		if (registered.recvThread != nullptr &&
			PushEventQueue(gameRequestQueue, event, hGameRequestGate, hGameRequestReady)) {
			SetEvent(hConnectionGate);
			continue;
		}

		const DWORD error = GetLastError();
		registered.state = ConnectionState::Stopping;
		SetEvent(hConnectionGate);

		// 생성된 스레드가 연결 목록을 확인하고 끝날 때까지 기다린다.
		if (registered.sendThread != nullptr)
			WaitForSingleObject(registered.sendThread, INFINITE);
		if (registered.recvThread != nullptr)
			WaitForSingleObject(registered.recvThread, INFINITE);

		if (WaitForSingleObject(hConnectionGate, INFINITE) != WAIT_OBJECT_0)
			return GetLastError();
		const TcpConnection failedConnection = registered;
		connections.erase(playerId);
		SetEvent(hConnectionGate);
		CloseTcpConnection(failedConnection);
		return error;
	}

}
