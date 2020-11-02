#pragma once
#include <optional>
#include <string>
#include "snowflake.hpp"
#include "json.hpp"

// unstable

enum class StickerFormatType {
    PNG = 1,
    APNG = 2,
    LOTTIE = 3,
};

struct Sticker {
    Snowflake ID;
    Snowflake PackID;
    std::string Name;
    std::string Description;
    std::optional<std::string> Tags;
    std::optional<std::string> AssetHash;
    std::optional<std::string> PreviewAssetHash;
    StickerFormatType FormatType;

    friend void from_json(const nlohmann::json &j, Sticker &m);

    std::string GetURL() const;
};
