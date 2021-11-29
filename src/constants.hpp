#include <cstdint>

constexpr static uint64_t SnowflakeSplitDifference = 600;
constexpr static int MaxMessagesForChatCull = 50; // this has to be 50 (for now) cuz that magic number is used in a couple other places and i dont feel like replacing them
