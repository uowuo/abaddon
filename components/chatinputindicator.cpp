#include <filesystem>
#include "chatinputindicator.hpp"
#include "../abaddon.hpp"
#include "../util.hpp"

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
    if (!std::filesystem::exists("./res/typing_indicator.gif")) return;
    auto gif_data = ReadWholeFile("./res/typing_indicator.gif");
    auto loader = Gdk::PixbufLoader::create();
    loader->signal_size_prepared().connect([&](int inw, int inh) {
        int w, h;
        GetImageDimensions(inw, inh, w, h, 20, 10);
        loader->set_size(w, h);
    });
    loader->write(gif_data.data(), gif_data.size());
    try {
        loader->close();
        m_img.property_pixbuf_animation() = loader->get_animation();
    } catch (const std::exception &) {}
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

void ChatInputIndicator::OnMessageCreate(Snowflake message_id) {
    const auto msg = Abaddon::Get().GetDiscordClient().GetMessage(message_id);
    if (!msg.has_value()) return;
    m_typers[msg->ChannelID].erase(msg->Author.ID);
    ComputeTypingString();
}

void ChatInputIndicator::SetTypingString(const Glib::ustring &str) {
    m_label.set_text(str);
    if (str == "")
        m_img.hide();
    else if (m_img.property_pixbuf_animation().get_value())
        m_img.show();
}

void ChatInputIndicator::ComputeTypingString() {
    if (m_custom_markup != "") {
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
    if (typers.size() == 0) {
        SetTypingString("");
    } else if (typers.size() == 1) {
        SetTypingString(typers[0].Username + " is typing...");
    } else if (typers.size() == 2) {
        SetTypingString(typers[0].Username + " and " + typers[1].Username + " are typing...");
    } else if (typers.size() > 2 && typers.size() <= MaxUsersInIndicator) {
        Glib::ustring str;
        for (int i = 0; i < typers.size() - 1; i++)
            str += typers[i].Username + ", ";
        SetTypingString(str + "and " + typers[typers.size() - 1].Username + " are typing...");
    } else { // size() > MaxUsersInIndicator
        SetTypingString("Several people are typing...");
    }
}
