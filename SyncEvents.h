#pragma once
#include <WinSock2.h>
#include <Windows.h>

// 큐마다 gate/ready 핸들 변수를 준비해서 호출
// 프로그램 공통 이벤트와 연결별 이벤트는 필요한 호출에서만 주소를 전달
// 전달하는 핸들은 서로 다른 변수여야 하며 모두 nullptr로 초기화되어 있어야 함
// 작업 스레드를 시작하기 전에 호출
bool InitSyncEvents(HANDLE& hQueueGate, HANDLE& hQueueReady,
    HANDLE* hConnectionGate = nullptr,
    HANDLE* hStopEvent = nullptr,
    HANDLE* hDisconnectEvent = nullptr);