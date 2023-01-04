#include "voiceinfobox.hpp"
#include "abaddon.hpp"
#include "util.hpp"

VoiceInfoBox::VoiceInfoBox()
    : Gtk::Box(Gtk::ORIENTATION_HORIZONTAL)
    , m_left(Gtk::ORIENTATION_VERTICAL) {
    m_disconnect_ev.signal_button_press_event().connect([this](GdkEventButton *ev) -> bool {
        if (ev->type == GDK_BUTTON_PRESS && ev->button == GDK_BUTTON_PRIMARY) {
            spdlog::get("discord")->debug("Request disconnect from info box");
            Abaddon::Get().GetDiscordClient().DisconnectFromVoice();
            return true;
        }

        return false;
    });

    AddPointerCursor(m_disconnect_ev);

    get_style_context()->add_class("voice-info");
    m_status.get_style_context()->add_class("voice-info-status");
    m_location.get_style_context()->add_class("voice-info-location");
    m_disconnect_img.get_style_context()->add_class("voice-info-disconnect-image");

    m_status.set_label("You shouldn't see me");
    m_location.set_label("You shouldn't see me");

    Abaddon::Get().GetDiscordClient().signal_voice_requested_connect().connect([this](Snowflake channel_id) {
        show();

        if (const auto channel = Abaddon::Get().GetDiscordClient().GetChannel(channel_id); channel.has_value() && channel->Name.has_value()) {
            if (channel->GuildID.has_value()) {
                if (const auto guild = Abaddon::Get().GetDiscordClient().GetGuild(*channel->GuildID); guild.has_value()) {
                    m_location.set_label(*channel->Name + " / " + guild->Name);
                    return;
                }
            }

            m_location.set_label(*channel->Name);
            return;
        }

        m_location.set_label("Unknown");
    });

    Abaddon::Get().GetDiscordClient().signal_voice_requested_disconnect().connect([this]() {
        hide();
    });

    Abaddon::Get().GetDiscordClient().signal_voice_client_state_update().connect([this](DiscordVoiceClient::State state) {
        Glib::ustring label;
        switch (state) {
            case DiscordVoiceClient::State::ConnectingToWebsocket:
                label = "Connecting";
                break;
            case DiscordVoiceClient::State::EstablishingConnection:
                label = "Establishing connection";
                break;
            case DiscordVoiceClient::State::Connected:
                label = "Connected";
                break;
            case DiscordVoiceClient::State::DisconnectedByServer:
            case DiscordVoiceClient::State::DisconnectedByClient:
                label = "Disconnected";
                break;
            default:
                label = "Unknown";
                break;
        }
        m_status.set_label(label);
    });

    m_status.set_ellipsize(Pango::ELLIPSIZE_END);
    m_location.set_ellipsize(Pango::ELLIPSIZE_END);

    m_disconnect_ev.add(m_disconnect_img);
    m_disconnect_img.property_icon_name() = "call-stop-symbolic";
    m_disconnect_img.property_icon_size() = 5;
    m_disconnect_img.set_hexpand(true);
    m_disconnect_img.set_halign(Gtk::ALIGN_END);

    m_left.add(m_status);
    m_left.add(m_location);

    add(m_left);
    add(m_disconnect_ev);

    show_all_children();
}
