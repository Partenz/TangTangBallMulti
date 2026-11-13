#include "ServerCollision.h"
#include "ServerVariable.h"

extern GameState gameState;

// 블럭의 타입
namespace {
	constexpr BYTE racingphase = 3;

	constexpr BYTE basicBlock = 0;
	constexpr BYTE trornBlock = 1;
	constexpr BYTE onceBlock = 2;
	constexpr BYTE jumpBlock = 3;
	constexpr BYTE starBlock = 4;
	constexpr BYTE doubleBlock = 5;
	constexpr BYTE lightningOnBlock = 7;
	constexpr BYTE lightningOffBlock = 8;

	// 공 크기와 시간 초
	constexpr float ballRadius = 10.0f;
	constexpr float tickSeconds = 0.015f;

    // 반동 속도 일단 0으로 초기화
    constexpr float normalBounceSpeed = 0.0f;
    constexpr float strongBounceSpeed = 0.0f;

	enum ContactType : BYTE {
		trornContact = 0,
		lightningContact = 1,
		onceBlockContact = 2,
		doubleBlockContact = 3,
		starContact = 4
	};
}

void ApplyContact(std::uint8_t playerId, std::uint32_t objectId, std::uint8_t contactType)
{
	// 게임이 진행중이 아니거나 라운드 승자가 결정된 상태라면 리턴함.
	if (gameState.phase != racingphase || gameState.roundWinnerId != 0) return;

	// 플레이어를 찾고 없거나 살아있지 않다면 리턴함.
	const auto it = gameState.players.find(playerId);
	if (it == gameState.players.end() || !it->second.alive) return;

	// 플레이어 상태
	PlayerState& player = it->second;

	for (ObjectInfo& object : gameState.objects) {
		if (object.objectId != objectId || object.used) continue;

		switch (contactType)
		{
		case trornContact:
			if (object.blockType != trornBlock) return;
			player.alive = false;
			break;

		case lightningContact:
			if (object.blockType != lightningOnBlock) return;
			player.alive = false;
			break;

		case onceBlockContact:
			if (object.blockType != onceBlock) return;
			object.used = true;
			break;

		case doubleBlockContact:
			if (object.blockType != doubleBlock) return;
			object.used = true;
			player.doubleJumpReady = true;
			break;

		case starContact:
			if (object.blockType != starBlock) return;
			gameState.roundWinnerId = playerId;
			break;

		default:
			return;
		}
	
		// 이 부분은 EndRound() 부분
		return;
	}
}

void CheckCollisions()
{
    if (gameState.phase != racingphase || gameState.roundWinnerId != 0) {
        return;
    }

    for (auto& [playerId, player] : gameState.players) {
        if (!player.alive) {
            continue;
        }

        // 공의 collisionbox
        auto& motion = player.motion;
        const float left = motion.x - ballRadius;
        const float right = motion.x + ballRadius;
        const float top = motion.y - ballRadius;
        const float bottom = motion.y + ballRadius;
        const float previousBottom = bottom - motion.vy * tickSeconds;

        const ObjectInfo* landingBlock = nullptr;

        for (const ObjectInfo& object : gameState.objects) {
            if (object.used || right < object.left || left > object.right) {
                continue;
            }

            // 하강 중 공의 아랫면이 블록 윗면을 지났는지 검사
            const bool isBounceBlock = object.blockType == basicBlock ||
                object.blockType == jumpBlock ||
                object.blockType == onceBlock;

            if (isBounceBlock) {
                if (motion.vy > 0.0f &&
                    previousBottom <= object.top && bottom >= object.top &&
                    (landingBlock == nullptr || object.top < landingBlock->top)) {
                    landingBlock = &object;
                }
                continue;
            }

            // 가시, 전기, 아이템, 별은 세로 범위도 겹쳐야 한다.
            if (bottom < object.top || top > object.bottom) {
                continue;
            }

            switch (object.blockType) {
            case trornBlock:
                ApplyContact(playerId, object.objectId, trornBlock);
                break;

            case lightningOnBlock:
                ApplyContact(playerId, object.objectId, lightningContact);
                break;

            case doubleBlock:
                ApplyContact(playerId, object.objectId, doubleBlock);
                break;

            case starBlock:
                ApplyContact(playerId, object.objectId, starContact);
                break;

            default:
                // OFF 전기 블록은 통과한다.
                break;
            }

            if (gameState.roundWinnerId != 0) {
                return;
            }

            if (!player.alive) {
                break;
            }
        }

        if (!player.alive || landingBlock == nullptr) {
            continue;
        }

        // 공을 블록 위로 맞춘 뒤 위쪽으로 튕긴다.
        motion.y = landingBlock->top - ballRadius;
        motion.vy = landingBlock->blockType == jumpBlock
            ? -strongBounceSpeed
            : -normalBounceSpeed;
        motion.falling = false;

        if (landingBlock->blockType == onceBlock) {
            ApplyContact(playerId, landingBlock->objectId, onceBlockContact);
        }
    }
}