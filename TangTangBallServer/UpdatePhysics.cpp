#include <cmath>
#include "UpdatePhysics.h"
#include "ServerVariable.h"
#include "ServerCollision.h"

extern GameState gameState;

void UpdatePhysics(float deltaSeconds)
{
    constexpr BYTE racingPhase = 3;
    constexpr std::uint8_t leftDirection = 1;
    constexpr std::uint8_t rightDirection = 2;
    constexpr float physicsStepSeconds = 0.015f;
    constexpr float horizontalSpeed = 140.0f;
    constexpr float gravity = 1000.0f;

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

            motion.vy += gravity * physicsStepSeconds;
            motion.x += motion.vx * physicsStepSeconds;
            motion.y += motion.vy * physicsStepSeconds;
            motion.falling = motion.vy > 0.0f;
        }

        CheckCollisions();

        if (gameState.phase != racingPhase or gameState.roundWinnerId != 0) {
            accumulatedSeconds = 0.0;
            break;
        }
    }
}