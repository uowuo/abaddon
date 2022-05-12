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

struct StickerData {
    Snowflake ID;
    Snowflake PackID;
    std::string Name;
    std::string Description;
    std::optional<std::string> Tags;
    std::optional<std::string> AssetHash;
    std::optional<std::string> PreviewAssetHash;
    StickerFormatType FormatType;

    friend void to_json(nlohmann::json &j, const StickerData &m);
    friend void from_json(const nlohmann::json &j, StickerData &m);
};

struct StickerItem {
    StickerFormatType FormatType;
    Snowflake ID;
    std::string Name;

    friend void to_json(nlohmann::json &j, const StickerItem &m);
    friend void from_json(const nlohmann::json &j, StickerItem &m);

    [[nodiscard]] std::string GetURL() const;
};
