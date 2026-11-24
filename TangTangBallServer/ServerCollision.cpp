#include "ServerCollision.h"
#include "ServerConnections.h"
#include "ServerVariable.h"
#include "UpdatePhysics.h"

#include <algorithm>

namespace {
    constexpr float ballRadius = 10.0f;
    constexpr float landingHalfWidth = ballRadius / 4.0f;
    constexpr float lightningMargin = 4.0f;
    constexpr BYTE contactEventType = 30;

    enum BlockType : BYTE {
        basic = 0, thorn = 1, once = 2, jump = 3, star = 4,
        doubleJump = 5, upDown = 6, lightningOn = 7
    };

    enum class Contact : BYTE {
        Thorn = 0, Lightning = 1, Once = 2, DoubleJump = 3, Goal = 4
    };

    bool IsSolid(const ObjectInfo& object)
    {
        return object.blockType == basic || object.blockType == once || object.blockType == jump;
    }

    void ApplyContact(BYTE playerId, PlayerState& player, ObjectInfo& object, Contact contact)
    {
        switch (contact) {
        case Contact::Thorn:
        case Contact::Lightning:
            player.alive = false;
            player.doubleJumpReady = false;
            break;
        case Contact::Once:
            object.used = true;
            break;
        case Contact::DoubleJump:
            object.used = true;
            player.doubleJumpReady = true;
            break;
        case Contact::Goal:
            object.used = true;
            gameState.roundWinnerId = playerId;
            break;
        }

        const uint32_t objectId = object.objectId;
        const std::vector<BYTE> body{
            static_cast<BYTE>(gameState.roundCnt >> 8), static_cast<BYTE>(gameState.roundCnt), playerId,
            static_cast<BYTE>(objectId >> 24), static_cast<BYTE>(objectId >> 16),
            static_cast<BYTE>(objectId >> 8), static_cast<BYTE>(objectId), static_cast<BYTE>(contact)
        };
        for (const auto& entry : gameState.players) {
            if (!QueueServerTcpPacket(entry.first, contactEventType, body))
                RequestDisconnect(entry.first);
        }
    }

    // Resolve each axis against its nearest surface before checking item contacts.
    void ResolveHorizontal(PlayerMotion& motion, float previousX, float previousY)
    {
        const bool movingRight = motion.x > previousX;
        float resolvedX = motion.x;
        for (const ObjectInfo& object : gameState.objects) {
            // 일반·일회용·점프·업다운 블록의 옆면을 막는다.
            if (object.used || (!IsSolid(object) && object.blockType != upDown))
                continue;

            const float ballTop = previousY - ballRadius;
            const float ballBottom = previousY + ballRadius;

            // 공과 블록의 높이가 겹칠 때만 옆면 충돌을 검사한다.
            if (ballBottom <= object.top || ballTop >= object.bottom)
                continue;

            if (movingRight && previousX + ballRadius <= object.left && resolvedX + ballRadius >= object.left)
                resolvedX = object.left - ballRadius;
            else if (motion.x < previousX && previousX - ballRadius >= object.right && resolvedX - ballRadius <= object.right)
                resolvedX = object.right + ballRadius;
        }
        if (resolvedX != motion.x)
            motion.vx = 0.0f;
        motion.x = resolvedX;
    }

    ObjectInfo* FindVerticalSurface(const PlayerMotion& motion, float previousY, bool falling)
    {
        ObjectInfo* nearest = nullptr;
        for (ObjectInfo& object : gameState.objects) {
            if (object.used)
                continue;

            // 기본값은 기존 착지 판정 폭인 2.5px다.
            float halfWidth = landingHalfWidth;

            if (IsSolid(object)) {
                halfWidth = ballRadius;
            }
            else if (object.blockType == thorn) {
                // 공이 위로 올라갈 때 가시 아랫면을 천장으로 검사한다.
                if (motion.y >= previousY)
                    continue;

                halfWidth = ballRadius;
            }
            else if (object.blockType == upDown) {
                if (falling)
                    continue;
            }
            else if (!IsSolid(object)) {
                continue;
            }

            if (object.blockType == thorn) {
                // 가시 아랫면은 모서리에 정확히 닿은 경우도 포함한다.
                if (motion.x + halfWidth < object.left ||
                    motion.x - halfWidth > object.right)
                    continue;
            }
            else {
                if (motion.x + halfWidth <= object.left ||
                    motion.x - halfWidth >= object.right)
                    continue;
            }

            if (falling && previousY + ballRadius <= object.top && motion.y + ballRadius >= object.top &&
                (nearest == nullptr || object.top < nearest->top))
                nearest = &object;
            else if (motion.y < previousY && previousY - ballRadius >= object.bottom && motion.y - ballRadius <= object.bottom &&
                (nearest == nullptr || object.bottom > nearest->bottom))
                nearest = &object;
        }
        return nearest;
    }

    bool TouchesAlongPath(const ObjectInfo& object, float previousX, float previousY, const PlayerMotion& motion)
    {
        const float margin = object.blockType == lightningOn ? lightningMargin : 0.0f;
        const auto overlaps = [&](float left, float top, float right, float bottom) {
            if (object.blockType == thorn && top - ballRadius >= object.bottom)
                return false;
            return right + ballRadius >= object.left + margin && left - ballRadius <= object.right - margin &&
                bottom + ballRadius >= object.top + margin && top - ballRadius <= object.bottom - margin;
        };
        return overlaps((std::min)(previousX, motion.x), previousY, (std::max)(previousX, motion.x), previousY) ||
            overlaps(motion.x, (std::min)(previousY, motion.y), motion.x, (std::max)(previousY, motion.y));
    }

    std::optional<Contact> GetTouchContact(BYTE blockType)
    {
        switch (blockType) {
        case thorn: return Contact::Thorn;
        case lightningOn: return Contact::Lightning;
        case doubleJump: return Contact::DoubleJump;
        case star: return Contact::Goal;
        default: return std::nullopt;
        }
    }
}

void ResolvePlayerCollisions(BYTE playerId, PlayerState& player, float previousX, float previousY)
{
    PlayerMotion& motion = player.motion;
    ResolveHorizontal(motion, previousX, previousY);
    const bool falling = motion.y > previousY;
    ObjectInfo* surface = FindVerticalSurface(motion, previousY, falling);
    if (surface != nullptr && surface->blockType != upDown)
        motion.y = falling ? surface->top - ballRadius : surface->bottom + ballRadius;

    for (ObjectInfo& object : gameState.objects) {
        if (object.used)
            continue;

        const auto contact = GetTouchContact(object.blockType);
        if (contact && TouchesAlongPath(object, previousX, previousY, motion)) {
            ApplyContact(playerId, player, object, *contact);
            if (!player.alive || gameState.roundWinnerId != 0)
                return;
        }
    }

    if (surface != nullptr) {
        if (falling || surface->blockType == upDown) {
            motion.vy = -JumpSpeedForCells(surface->blockType == jump ? 3 : 2);
            if (surface->blockType == once)
                ApplyContact(playerId, player, *surface, Contact::Once);
        } else {
            motion.vy = 0.0f;
        }
    }
    motion.falling = motion.vy > 0.0f;

    if (gameState.mapHeight != 0 && motion.y - ballRadius > gameState.mapHeight) {
        player.alive = false;
        player.doubleJumpReady = false;
    }
}
