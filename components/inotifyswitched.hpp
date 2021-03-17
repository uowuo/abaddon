#pragma once

// for things that are used in stackswitchers to be able to be told when they're switched to

class INotifySwitched {
public:
    virtual void on_switched_to() {};
};
