#include "chatinputindicator.hpp"
#include <filesystem>
#include <fmt/format.h>
#include <gdkmm/pixbufloader.h>
#include <glibmm/i18n.h>
#include "abaddon.hpp"
#include "util.hpp"

constexpr static const int MaxUsersInIndicator = 4;

ChatInputIndicator::ChatInputIndicator()
    : Gtk::Box(Gtk::ORIENTATION_HORIZONTAL) {
    m_label.set_text("");
    m_label.set_ellipsize(Pango::ELLIPSIZE_END);
    m_label.set_valign(Gtk::ALIGN_END);
    m_img.set_margin_right(5);
    get_style_context()->add_class("typing-indicator");

    Abaddon::Get().GetDiscordClient().signal_typing_start().connect(sigc::mem_fun(*this, &ChatInputIndicator::OnUserTypingStart));
    Abaddon::Get().GetDiscordClient().signal_message_create().connect(sigc::mem_fun(*this, &ChatInputIndicator::OnMessageCreate));

    add(m_img);
    add(m_label);
    m_label.show();

    // try loading gif
    const static auto path = Abaddon::GetResPath("/typing_indicator.gif");
    if (!std::filesystem::exists(path)) return;
    auto gif_data = ReadWholeFile(path);
    auto loader = Gdk::PixbufLoader::create();
    try {
        loader->signal_size_prepared().connect([&](int inw, int inh) {
            int w, h;
            GetImageDimensions(inw, inh, w, h, 20, 10);
            loader->set_size(w, h);
        });
        loader->write(gif_data.data(), gif_data.size());
        loader->close();
        m_img.property_pixbuf_animation() = loader->get_animation();
    } catch (...) {}
}

void ChatInputIndicator::AddUser(Snowflake channel_id, const UserData &user, int timeout) {
    auto current_connection_it = m_typers[channel_id].find(user.ID);
    if (current_connection_it != m_typers.at(channel_id).end()) {
        current_connection_it->second.disconnect();
        m_typers.at(channel_id).erase(current_connection_it);
    }

    Snowflake user_id = user.ID;
    auto cb = [this, user_id, channel_id]() -> bool {
        m_typers.at(channel_id).erase(user_id);
        ComputeTypingString();
        return false;
    };
    m_typers[channel_id][user.ID] = Glib::signal_timeout().connect_seconds(cb, timeout);
    ComputeTypingString();
}

void ChatInputIndicator::SetActiveChannel(Snowflake id) {
    m_active_channel = id;
    const auto channel = Abaddon::Get().GetDiscordClient().GetChannel(id);
    if (channel.has_value()) {
        m_active_guild = channel->GuildID;
    } else {
        m_active_guild = std::nullopt;
    }
    ComputeTypingString();
}

void ChatInputIndicator::SetCustomMarkup(const Glib::ustring &str) {
    m_custom_markup = str;
    ComputeTypingString();
}

void ChatInputIndicator::ClearCustom() {
    m_custom_markup = "";
    ComputeTypingString();
}

void ChatInputIndicator::OnUserTypingStart(Snowflake user_id, Snowflake channel_id) {
    const auto &discord = Abaddon::Get().GetDiscordClient();
    const auto user = discord.GetUser(user_id);
    if (!user.has_value()) return;

    AddUser(channel_id, *user, 10);
}

void ChatInputIndicator::OnMessageCreate(const Message &message) {
    m_typers[message.ChannelID].erase(message.Author.ID);
    ComputeTypingString();
}

void ChatInputIndicator::SetTypingString(const Glib::ustring &str) {
    m_label.set_text(str);
    if (str.empty())
        m_img.hide();
    else if (m_img.property_pixbuf_animation().get_value())
        m_img.show();
}

void ChatInputIndicator::ComputeTypingString() {
    if (!m_custom_markup.empty()) {
        m_label.set_markup(m_custom_markup);
        m_img.hide();
        return;
    }

    const auto &discord = Abaddon::Get().GetDiscordClient();
    std::vector<UserData> typers;
    for (const auto &[id, conn] : m_typers[m_active_channel]) {
        const auto user = discord.GetUser(id);
        if (user.has_value())
            typers.push_back(*user);
    }
    if (typers.empty()) {
        SetTypingString("");
    } else if (typers.size() == 1) {
        SetTypingString(Glib::ustring::compose(_("%1 is typing..."), typers[0].GetDisplayName(m_active_guild)));
    } else if (typers.size() == 2) {
        SetTypingString(Glib::ustring::compose(_("%1 and %2 are typing"),
                                               typers[0].GetDisplayName(m_active_guild),
                                               typers[1].GetDisplayName(m_active_guild)));
    } else if (typers.size() > 2 && typers.size() <= MaxUsersInIndicator) {
        // FIXME: Just because this suffice the case where 4 are typing, does not necessarily mean that there are exactly 4 people typing
        Glib::ustring str = Glib::ustring::compose(_("%1, %2, %3 and %4 are typing..."),
                                                   typers[0].GetDisplayName(m_active_guild),
                                                   typers[1].GetDisplayName(m_active_guild),
                                                   typers[2].GetDisplayName(m_active_guild),
                                                   typers[3].GetDisplayName(m_active_guild));
        SetTypingString(str);
    } else { // size() > MaxUsersInIndicator
        SetTypingString(_("Several people are typing..."));
    }
}
