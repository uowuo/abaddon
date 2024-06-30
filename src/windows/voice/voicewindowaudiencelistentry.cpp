#include "voicewindowaudiencelistentry.hpp"
#include "abaddon.hpp"

VoiceWindowAudienceListEntry::VoiceWindowAudienceListEntry(Snowflake id)
    : m_main(Gtk::ORIENTATION_HORIZONTAL)
    , m_avatar(32, 32) {
    m_name.set_halign(Gtk::ALIGN_START);
    m_name.set_hexpand(true);

    m_main.add(m_avatar);
    m_main.add(m_name);
    add(m_main);
    show_all_children();

    auto &discord = Abaddon::Get().GetDiscordClient();
    const auto user = discord.GetUser(id);
    if (user.has_value()) {
        m_name.set_text(user->GetUsername());
        m_avatar.SetURL(user->GetAvatarURL("png", "32"));
    } else {
        m_name.set_text("Unknown user");
    }
}
