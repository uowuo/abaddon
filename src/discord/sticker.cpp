#include "sticker.hpp"

void to_json(nlohmann::json &j, const StickerData &m) {
    j["id"] = m.ID;
    j["pack_id"] = m.PackID;
    j["name"] = m.Name;
    j["description"] = m.Description;
    JS_IF("tags", m.Tags);
    JS_IF("asset", m.AssetHash);
    JS_IF("preview_asset", m.PreviewAssetHash);
    j["format_type"] = m.FormatType;
}

void from_json(const nlohmann::json &j, StickerData &m) {
    JS_D("id", m.ID);
    JS_D("pack_id", m.PackID);
    JS_D("name", m.Name);
    JS_D("description", m.Description);
    JS_O("tags", m.Tags);
    JS_O("asset", m.AssetHash);
    JS_ON("preview_asset", m.PreviewAssetHash);
    JS_D("format_type", m.FormatType);
}

void to_json(nlohmann::json &j, const StickerItem &m) {
    j["id"] = m.ID;
    j["name"] = m.Name;
    j["format_type"] = m.FormatType;
}

void from_json(const nlohmann::json &j, StickerItem &m) {
    JS_D("id", m.ID);
    JS_D("name", m.Name);
    JS_D("format_type", m.FormatType);
}

std::string StickerItem::GetURL() const {
    if (FormatType == StickerFormatType::PNG || FormatType == StickerFormatType::APNG)
        return "https://media.discordapp.net/stickers/" + std::to_string(ID) + ".png?size=256";
    else if (FormatType == StickerFormatType::LOTTIE)
        return "https://media.discordapp.net/stickers/" + std::to_string(ID) + ".json";
    return "";
}
