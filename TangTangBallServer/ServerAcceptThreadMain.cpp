#include "ServerConnections.h"
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

		// 종료 이벤트 확인
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

		// disconnect 이벤트 생성
		HANDLE disconnectEvent = CreateEvent(nullptr, TRUE, FALSE, nullptr);
		if (disconnectEvent == nullptr)
		{
			const int err = GetLastError();
			closesocket(clientSocket);
			return err;
		}

		// singleObject로 연결 제어
		if (WaitForSingleObject(hConnectionGate, INFINITE) != WAIT_OBJECT_0) {
			const int err = GetLastError();
			closesocket(clientSocket);
			CloseHandle(disconnectEvent);
			return err;
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
			CloseHandle(disconnectEvent);
			closesocket(clientSocket);
			continue;
		}

		TcpConnection connection{};
		connection.socket = clientSocket;
		connection.disconnectEvent = disconnectEvent;
		
		connections.emplace(playerId, connection);
		// 참가자별 컨텍스트
		connection.sendContext = std::make_shared<ServerTcpSendContext>();

		SetEvent(hConnectionGate);

		// 연결 등록을 마친 뒤 게임 요청 큐에 새 접속을 알린다.
		NetEvent event{};
		event.eventKind = NetEventConnected;
		event.playerId = playerId;

		if (!PushEventQueue(gameRequestQueue, event,
			hGameRequestGate, hGameRequestReady)) {
			return GetLastError();
		}

	}

}