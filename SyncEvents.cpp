#include "SyncEvents.h"

bool InitSyncEvents(HANDLE& hQueueGate, HANDLE& hQueueReady,
    HANDLE* hConnectionGate, HANDLE* hStopEvent,
    HANDLE* hDisconnectEvent)
{
    // Gate: 자동 리셋/초기 신호, Ready/Stop/Disconnect: 수동 리셋/초기 비신호.
    HANDLE* handles[] = {
        &hQueueGate, &hQueueReady, hConnectionGate, hStopEvent, hDisconnectEvent
    };
    const BOOL manualReset[] = { FALSE, TRUE, FALSE, TRUE, TRUE };
    const BOOL initialState[] = { TRUE, FALSE, TRUE, FALSE, FALSE };

    // 생성 전에 잘못된 인수와 중복 초기화를 확인
    for (int i = 0; i < 5; ++i) {
        if (handles[i] == nullptr) continue;
        for (int j = 0; j < i; ++j) {
            if (handles[i] == handles[j]) {
                SetLastError(ERROR_INVALID_PARAMETER);
                return false;
            }
        }
        if (*handles[i] != nullptr) {
            SetLastError(ERROR_ALREADY_INITIALIZED);
            return false;
        }
    }

    for (int i = 0; i < 5; ++i) {
        if (handles[i] == nullptr) continue;
        *handles[i] = CreateEventW(nullptr, manualReset[i], initialState[i], nullptr);
        if (*handles[i] == nullptr) {
            const DWORD error = GetLastError();
            // 이번 호출에서 생성하다 실패한 핸들만 되돌린다
            for (int j = 0; j < i; ++j) {
                if (handles[j] != nullptr) {
                    CloseHandle(*handles[j]);
                    *handles[j] = nullptr;
                }
            }
            SetLastError(error);
            return false;
        }
    }
    return true;
}