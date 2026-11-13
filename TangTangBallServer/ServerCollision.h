#pragma once
#include <cstdint>

// 공 위치와 맵 객체 범위가 겹치는지 확인
void CheckCollisions();

// 플레이어의 공이 맵 객체와 충돌했는지 확인
void ApplyContact(std::uint8_t playgerId, std::uint32_t objectId, std::uint8_t contactType);