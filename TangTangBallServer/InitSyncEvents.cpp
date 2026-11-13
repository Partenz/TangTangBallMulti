#include "InitSyncEvents.h"
#include "CloseSyncEvents.h"
#include "ServerVariable.h"

bool InitSyncEvents()
{
    HANDLE* handles[] = { &hQueueGate, &hQueueReady, &hConnectionGate, &hStopEvent, &hDisconnectEvent };
    const BOOL manualReset[] = { FALSE, TRUE,  FALSE, TRUE,  TRUE };
    const BOOL initialSignal[] = { TRUE,  FALSE, TRUE,  FALSE, FALSE };

    for (HANDLE* handle : handles) {
        if (*handle != nullptr) {
            SetLastError(ERROR_ALREADY_INITIALIZED);
            return false;
        }
    }

    for (std::size_t i = 0; i < sizeof(handles) / sizeof(handles[0]); ++i) {
        *handles[i] = CreateEventW(
            nullptr, manualReset[i], initialSignal[i], nullptr);

        if (*handles[i] == nullptr) {
            const DWORD error = GetLastError();
            CloseSyncEvents();
            SetLastError(error);
            return false;
        }
    }

    return true;
}