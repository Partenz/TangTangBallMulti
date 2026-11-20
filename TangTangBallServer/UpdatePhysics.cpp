#include <cmath>
#include <algorithm>
#include "EndRound.h"
#include "UpdatePhysics.h"
#include "ServerVariable.h"
#include "ServerCollision.h"

extern GameState gameState;

namespace {
    constexpr float physicsStepSeconds = 0.015f;
    constexpr float gravity = 1000.0f;
    constexpr float cellSize = 40.0f;
}

float JumpSpeedForCells(int heightInCells)
{
    // Gravity is applied before position each step, so compensate for half a step.
    return std::sqrt(2.0f * gravity * cellSize * heightInCells) + gravity * physicsStepSeconds / 2.0f;
}

void UpdatePhysics(float deltaSeconds)
{
    constexpr BYTE racingPhase = 3;
    constexpr std::uint8_t leftDirection = 1;
    constexpr std::uint8_t rightDirection = 2;
    constexpr float horizontalSpeed = 140.0f;

    static double accumulatedSeconds = 0.0;
    static std::uint16_t physicsRoundCnt = 0;

    if (gameState.phase != racingPhase or gameState.roundWinnerId != 0) {
        accumulatedSeconds = 0.0;
        return;
    }

    if (physicsRoundCnt != gameState.roundCnt) {
        physicsRoundCnt = gameState.roundCnt;
        accumulatedSeconds = 0.0;
    }

    if (not std::isfinite(deltaSeconds) or deltaSeconds <= 0.0f) {
        return;
    }

    accumulatedSeconds += deltaSeconds;

    while (accumulatedSeconds >= physicsStepSeconds) {
        accumulatedSeconds -= physicsStepSeconds;

        for (auto& entry : gameState.players) {
            PlayerState& player = entry.second;
            if (not player.alive) {
                continue;
            }

            PlayerMotion& motion = player.motion;
            motion.vx = 0.0f;

            if (player.moveDirection == leftDirection) {
                motion.vx = -horizontalSpeed;
            }
            else if (player.moveDirection == rightDirection) {
                motion.vx = horizontalSpeed;
            }

            const float previousX = motion.x;
            const float previousY = motion.y;
            motion.vy += gravity * physicsStepSeconds;
            motion.x += motion.vx * physicsStepSeconds;
            motion.y += motion.vy * physicsStepSeconds;
            ResolvePlayerCollisions(entry.first, player, previousX, previousY);
            if (gameState.roundWinnerId != 0)
                break;
        }

        const bool allEliminated = !gameState.players.empty() &&
            std::none_of(gameState.players.begin(), gameState.players.end(),
                [](const auto& entry) { return entry.second.alive; });
        if (gameState.roundWinnerId != 0 || allEliminated) {
            EndRound(gameState.roundWinnerId);
            accumulatedSeconds = 0.0;
            break;
        }
    }
}
