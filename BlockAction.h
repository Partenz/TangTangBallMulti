#pragma once

#include <cstdint>

namespace BlockAction {
    constexpr std::uint8_t starBlock = 4;
    constexpr std::uint8_t deleteAssignment = 9;
    constexpr std::uint8_t deleteRequest = 26;
    constexpr std::uint8_t blockDeleted = 27;
}
