#include <cstdint>

bool IsNewerSequence(uint32_t received, uint32_t last)
{
    const uint32_t difference = received - last;

    return difference != 0 and difference < UINT32_MAX / 2u;
}