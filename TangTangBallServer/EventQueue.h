#pragma once
#include "ServerVariable.h"

// InitSyncEvents로 생성한 큐별 gate/ready를 전달합니다.
// gate: 자동 리셋, 초기 신호 / ready: 수동 리셋, 초기 비신호.
// 모든 큐 접근은 같은 gate로 보호하고, 모든 작업 스레드 종료 후 핸들을 닫습니다.
// 실패 시 false, 상세 오류는 GetLastError().
// 이벤트 API가 실패하면 큐가 이미 변경되었을 수 있으므로 무조건 재시도하지 않습니다.
bool PushEventQueue(std::queue<NetEvent>& queue, const NetEvent& event,
    HANDLE gate, HANDLE ready);

// 현재 큐의 모든 사건을 FIFO 순서로 events 뒤에 추가합니다.
// events는 호출 스레드 전용이어야 합니다. 게임 처리/송신은 반환 후 수행합니다.
bool DrainEventQueue(std::queue<NetEvent>& queue, std::vector<NetEvent>& events,
    HANDLE gate, HANDLE ready);
