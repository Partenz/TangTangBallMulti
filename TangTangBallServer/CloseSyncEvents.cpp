#include "CloseSyncEvents.h"

bool CloseSyncEvents(HANDLE& hQueueGate, HANDLE& hQueueReady, HANDLE* hConnectionGate, HANDLE* hStopEvent, HANDLE* hDisconnectEvent)
{
    HANDLE* handles[] = { &hQueueGate, &hQueueReady, hConnectionGate, hStopEvent, hDisconnectEvent};

    for (int i = 0; i < 5; ++i) {
        if (handles[i] == nullptr)
            continue;
        for (int j = 0; j < i; ++j) {
            if (handles[j] == nullptr)
                continue;
            if (handles[i] == handles[j] || (*handles[i] != nullptr && *handles[i] == *handles[j])) {
                SetLastError(ERROR_INVALID_PARAMETER);
                return false;
            }
        }
    }

    DWORD firstError = ERROR_SUCCESS;
    for (HANDLE* handle : handles) {
        if (handle == nullptr || *handle == nullptr)
            continue;
        if (CloseHandle(*handle)) {
            *handle = nullptr;
        } 
        else if (firstError == ERROR_SUCCESS) {
            firstError = GetLastError();
        }
    }

    if (firstError != ERROR_SUCCESS) {
        SetLastError(firstError);
        return false;
    }
    return true;
}
