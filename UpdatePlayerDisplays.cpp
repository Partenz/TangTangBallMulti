#include "UpdatePlayerDisplays.h"
#include "ClientVariable.h"

void UpdatePlayerDisplays(ULONGLONG now)
{
    constexpr BYTE racingPhase = 3;
    constexpr ULONGLONG maxPredictionMs = 100;

    static ULONGLONG lastDisplayUpdateMs = 0;
    const ULONGLONG previousDisplayUpdateMs = lastDisplayUpdateMs;
    lastDisplayUpdateMs = now;

    if (clientGameState.phase != racingPhase)
        return;

    for (auto& entry : clientGameState.players) {
        PlayerState& player = entry.second;
        if (!player.alive || player.lastMoveTime == 0)
            continue;

        const ULONGLONG predictionStartMs = (std::max)(previousDisplayUpdateMs, player.lastMoveTime);
        const ULONGLONG predictionEndMs = (std::min)(now, player.lastMoveTime + maxPredictionMs);

        if (predictionEndMs <= predictionStartMs)
            continue;

        const float elapsedSeconds = static_cast<float>(predictionEndMs - predictionStartMs) / 1000.0f;

        player.motion.x += player.motion.vx * elapsedSeconds;
        player.motion.y += player.motion.vy * elapsedSeconds;
    }
}