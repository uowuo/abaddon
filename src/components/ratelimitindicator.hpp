#pragma once
#include <unordered_map>
#include <chrono>

#include <gtkmm/box.h>
#include <gtkmm/image.h>
#include <gtkmm/label.h>

#include "discord/message.hpp"

class RateLimitIndicator : public Gtk::Box {
public:
    RateLimitIndicator();
    void SetActiveChannel(Snowflake id);

    // even tho this probably isnt the right place for this im gonna do it anyway to reduce coad
    bool CanSpeak() const;

private:
    int GetTimeLeft() const;
    int GetRateLimit() const;
    bool UpdateIndicator();
    void OnMessageCreate(const Message &message);
    void OnMessageSendFail(const std::string &nonce, float rate_limit);
    void OnChannelUpdate(Snowflake channel_id);

    Gtk::Image m_img;
    Gtk::Label m_label;

    sigc::connection m_connection;

    int m_rate_limit;
    Snowflake m_active_channel;
    std::unordered_map<Snowflake, std::chrono::time_point<std::chrono::steady_clock>> m_times; // time point of when next message can be sent
};
