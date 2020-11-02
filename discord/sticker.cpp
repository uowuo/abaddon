#include "sticker.hpp"

void from_json(const nlohmann::json &j, Sticker &m) {
    JS_D("id", m.ID);
    JS_D("pack_id", m.PackID);
    JS_D("name", m.Name);
    JS_D("description", m.Description);
    JS_O("tags", m.Tags);
    JS_D("asset", m.AssetHash);
    JS_N("preview_asset", m.PreviewAssetHash);
    JS_D("format_type", m.FormatType);
}

std::string Sticker::GetURL() const {
    if (!AssetHash.has_value()) return "";
    if (FormatType == StickerFormatType::PNG || FormatType == StickerFormatType::APNG)
        return "https://media.discordapp.net/stickers/" + std::to_string(ID) + "/" + *AssetHash + ".png?size=256";
    else if (FormatType == StickerFormatType::LOTTIE)
        return "https://media.discordapp.net/stickers/" + std::to_string(ID) + "/" + *AssetHash + ".json";
    return "";
}
