#pragma once
#include "notifier.hpp"

class Message;

class Notifications {
public:
    Notifications();

    void CheckMessage(const Message &message);

private:
    void NotifyMessage(const Message &message);

    [[nodiscard]] bool IsDND() const;

    Notifier m_notifier;
};
