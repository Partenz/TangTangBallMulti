#include "RegisterUDP.h"
#include "EnDePacket.h"
#include "ClientVariable.h"
#include "EventQueue.h"


bool RegisterUdp(uint8_t playerId, uint64_t udpKey)
{
    if (playerId == 0)
        return false;

    // playerId(1바이트) + udpKey(8바이트)
    std::vector<BYTE> body;
    body.reserve(9);
    body.push_back(playerId);

    // updKey는 큰 자리 바이트부터 저장
    for (int shift = 56; shift >= 0; shift -= 8)
    {
        body.push_back(static_cast<BYTE>((udpKey >> shift) & 0xFFu));
    }

    const BYTE UDP_REGISTER = 100;

    std::vector<BYTE> packet = EncodePacket(UDP_REGISTER, body);

    if (packet.empty())
        return false;

    // 큐 및 이벤트 초기화 코드와 UDP 송수신 스레드가
    // 구현된 뒤 아래 코드를 활성화한다.

    const int maxAttempts = 3;
    const DWORD retryIntervalMs = 1000;

    HANDLE handles[] = { hStopEvent, hUdpRegisteredEvent, hClientUdpSendGate, hClientUdpSendReady };

    for (int i = 0; i < 4; ++i)
    {
        if (handles[i] == nullptr ||
            handles[i] == INVALID_HANDLE_VALUE)
        {
            return false;
        }

        for (int j = 0; j < i; ++j)
        {
            if (handles[i] == handles[j])
                return false;
        }
    }

    NetEvent event{};
    event.playerId = playerId;
    event.packetType = UDP_REGISTER;

    // 송신 큐에는 헤더가 포함된 완성 패킷을 저장한다.
    // UDP 송신 스레드는 이를 다시 인코딩하지 않고 전송한다.
    event.packetData = packet;

    if (!ResetEvent(hUdpRegisteredEvent))
        return false;

    // 종료 이벤트를 먼저 배치하여 우선 처리한다.
    HANDLE waitEvents[] = { hStopEvent, hUdpRegisteredEvent };

    for (int attempt = 0; attempt < maxAttempts; ++attempt)
    {
        if (WaitForSingleObject(hStopEvent, 0) != WAIT_TIMEOUT)
            return false;

        // Gate 획득 → 큐 삽입 → Ready 설정 → Gate 반납
        if (!PushEventQueue(clientUdpSendQueue, event, hClientUdpSendGate, hClientUdpSendReady))
            return false;

        // 큐 Gate를 반납한 상태에서 등록 응답을 기다린다.
        // 수신 스레드는 정상 UDP_REGISTERED(101) 수신 시
        // SetEvent(hUdpRegisteredEvent)를 호출한다.
        DWORD result = WaitForMultipleObjects(2, waitEvents, FALSE, retryIntervalMs);

        if (result == WAIT_OBJECT_0 + 1)
            return true;

        if (result != WAIT_TIMEOUT)
            return false;

        // 시간 초과 시 재전송.
        // 늦은 응답을 놓치지 않도록 여기서는 ResetEvent를 하지 않는다.
    }

    return false;
}