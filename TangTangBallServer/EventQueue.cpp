#include "EventQueue.h"
#include <new>
#include <stdexcept>
#include <utility>

namespace
{
    bool AcquireQueueGate(HANDLE gate, HANDLE ready)
    {
        if (gate == nullptr || ready == nullptr || gate == INVALID_HANDLE_VALUE
            || ready == INVALID_HANDLE_VALUE || gate == ready) {
            SetLastError(ERROR_INVALID_PARAMETER);
            return false;
        }
        return WaitForSingleObject(gate, INFINITE) == WAIT_OBJECT_0;
    }

    bool ReleaseQueueGate(HANDLE gate, DWORD error = ERROR_SUCCESS)
    {
        const BOOL released = SetEvent(gate);
        if (error != ERROR_SUCCESS) {
            SetLastError(error); // gate 반납이 앞서 발생한 오류를 덮지 않도록 보존
            return false;
        }
        return released != FALSE;
    }
}

bool PushEventQueue(std::queue<NetEvent>& queue, const NetEvent& event,
    HANDLE gate, HANDLE ready)
{
    if (!AcquireQueueGate(gate, ready))
        return false;

    try {
        queue.push(event); // 호출자의 원본과 별도로 복사하여 보관
    }
    catch (const std::bad_alloc&) {
        return ReleaseQueueGate(gate, ERROR_NOT_ENOUGH_MEMORY);
    }

    // gate를 보유한 상태에서 알림을 설정해야 Drain의 ResetEvent와 엇갈리지 않습니다.
    const DWORD error = SetEvent(ready) ? ERROR_SUCCESS : GetLastError();
    return ReleaseQueueGate(gate, error);
}

bool DrainEventQueue(std::queue<NetEvent>& queue, std::vector<NetEvent>& events,
    HANDLE gate, HANDLE ready)
{
    if (!AcquireQueueGate(gate, ready))
        return false;

    if (queue.size() > events.max_size() - events.size())
        return ReleaseQueueGate(gate, ERROR_NOT_ENOUGH_MEMORY);

    try {
        // 먼저 필요한 공간을 확보합니다. 실패하면 큐의 항목은 그대로 남습니다.
        events.reserve(events.size() + queue.size());
    }
    catch (const std::bad_alloc&) {
        return ReleaseQueueGate(gate, ERROR_NOT_ENOUGH_MEMORY);
    }
    catch (const std::length_error&) {
        return ReleaseQueueGate(gate, ERROR_NOT_ENOUGH_MEMORY);
    }

    while (!queue.empty()) {
        events.push_back(std::move(queue.front()));
        queue.pop();
    }

    // 큐를 비운 뒤, 다른 생산자가 넣기 전에 ready를 비신호로 변경합니다.
    const DWORD error = ResetEvent(ready) ? ERROR_SUCCESS : GetLastError();
    return ReleaseQueueGate(gate, error);
}
