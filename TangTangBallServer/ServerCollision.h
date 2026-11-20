#pragma once
#include <cstdint>

struct PlayerState;

void ResolvePlayerCollisions(std::uint8_t playerId, PlayerState& player, float previousX, float previousY);
