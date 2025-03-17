#ifndef GLOBALHOTKEYMANAGER_H
#define GLOBALHOTKEYMANAGER_H

#pragma once

#include <map>
#include <functional>
#include <mutex>
#include "uiohook.h"

// hotkey callback type
using HotkeyCallback = std::function<void(void)>;

class GlobalHotkeyManager
{
public:
    static GlobalHotkeyManager& instance();
    int registerHotkey(uint16_t keycode, uint32_t modifiers, HotkeyCallback callback);
    void unregisterHotkey(int id);

private:
    GlobalHotkeyManager();
    ~GlobalHotkeyManager();

    // no copy/move
    GlobalHotkeyManager(const GlobalHotkeyManager&) = delete;
    GlobalHotkeyManager& operator=(const GlobalHotkeyManager&) = delete;

    struct Hotkey;
    Hotkey* find_hotkey(uint16_t keycode, uint32_t modifiers);

    static void hook_callback(uiohook_event* const event);
    void handleEvent(uiohook_event* const event);

    std::mutex m_mutex;
    std::map<int, Hotkey> m_callbacks;
    int m_nextId;
};

#endif