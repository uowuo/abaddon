#pragma once
#include "notifier.hpp"
#include <map>
#include <vector>

class Message;

class Notifications {
public:
    Notifications();

    void CheckMessage(const Message &message);
    void WithdrawChannel(Snowflake channel_id);

private:
    void NotifyMessageDM(const Message &message);
    void NotifyMessageGuild(const Message &message);

    [[nodiscard]] bool IsDND() const;

    Notifier m_notifier;
    std::map<Snowflake, std::vector<Snowflake>> m_chan_notifications;
};
