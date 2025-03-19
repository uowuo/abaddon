#ifndef GLOBALHOTKEYMANAGER_H
#define GLOBALHOTKEYMANAGER_H

#pragma once
#ifdef WITH_HOTKEYS

#include <map>
#include <functional>
#include <optional>
#include <tuple>
#include <mutex>
#include <glibmm/dispatcher.h>
#include "uiohook.h"

// hotkey callback type
using HotkeyCallback = std::function<void(void)>;

class GlobalHotkeyManager
{
public:
    static GlobalHotkeyManager& instance();
    int registerHotkey(uint16_t keycode, uint32_t modifiers, HotkeyCallback callback);
    int registerHotkey(const char *shortcut_str, HotkeyCallback callback);
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
    void processCallbacks();

    // Converting gtk to uiohook codes
    uint16_t convert_gdk_keyval_to_uihooks_key(guint keyval);
    uint32_t convert_gdk_modifiers_to_uihooks(GdkModifierType mods);
    std::optional<std::tuple<uint16_t, uint32_t>> parse_and_convert_shortcut(const char *shortcut_str);

    Glib::Dispatcher m_dispatcher;
    std::queue<HotkeyCallback> m_pendingCallbacks;
    std::mutex m_queueMutex;

    std::mutex m_mutex;
    std::map<int, Hotkey> m_callbacks;
    int m_nextId;
};

#endif
#endif