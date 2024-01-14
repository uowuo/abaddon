#pragma once
#include <cstdint>

constexpr static uint64_t SnowflakeSplitDifference = 600;
constexpr static int MaxMessagesForChatCull = 50; // this has to be 50 (for now) cuz that magic number is used in a couple other places and i dont feel like replacing them
constexpr static int AttachmentItemSize = 120;
constexpr static int BaseAttachmentSizeLimit = 25 * 1024 * 1024;
constexpr static int NitroClassicAttachmentSizeLimit = 50 * 1024 * 1024;
constexpr static int NitroAttachmentSizeLimit = 100 * 1024 * 1024;
constexpr static int BoostLevel2AttachmentSizeLimit = 50 * 1024 * 1024;
constexpr static int BoostLevel3AttachmentSizeLimit = 100 * 1024 * 1024;
constexpr static int MaxMessagePayloadSize = 199 * 1024 * 1024;
constexpr static int EmojiSize = 24; // settings eventually
constexpr static int AvatarSize = 32;
constexpr static int ThumbnailSize = 100;
constexpr static int StickerComponentSize = 160;
