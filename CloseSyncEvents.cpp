#include "CloseSyncEvents.h"
#include "ClientVariable.h"

bool CloseSyncEvents()
{
    HANDLE* handles[] = { &hQueueGate, &hQueueReady, &hConnectionGate, &hStopEvent, &hDisconnectEvent };

    DWORD firstError = ERROR_SUCCESS;

    for (HANDLE* handle : handles) {
        if (*handle == nullptr)
            continue;

        if (CloseHandle(*handle)) {
            *handle = nullptr;
        }
        else if (firstError == ERROR_SUCCESS) {
            firstError = GetLastError();
        }
    }

    clientTcpConnection.stopEvent = hStopEvent;
    clientTcpConnection.disconnectEvent = hDisconnectEvent;

    if (firstError != ERROR_SUCCESS) {
        SetLastError(firstError);
        return false;
    }

    return true;
}