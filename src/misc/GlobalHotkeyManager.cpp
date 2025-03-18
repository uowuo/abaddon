#include "sigc++/functors/mem_fun.h"
#ifdef WITH_HOTKEYS
#include "GlobalHotkeyManager.hpp"
#include "spdlog/spdlog.h"

// Singleton instance
GlobalHotkeyManager& GlobalHotkeyManager::instance() {
    static GlobalHotkeyManager instance;
    return instance;
}

GlobalHotkeyManager::GlobalHotkeyManager() : m_nextId(1) {
    m_dispatcher.connect(sigc::mem_fun(*this, &GlobalHotkeyManager::processCallbacks));

    hook_set_dispatch_proc(&GlobalHotkeyManager::hook_callback);

    // Run hook in separate thread to not block gtk
    std::thread([this]() {
        if (hook_run() != UIOHOOK_SUCCESS) {
            spdlog::get("ui")->error("Failed to start libuiohook");
        }
    }).detach();
}

void GlobalHotkeyManager::processCallbacks()
{
    std::queue<HotkeyCallback> callbacksToRun;
    {
        std::lock_guard<std::mutex> lock(m_queueMutex);
        std::swap(callbacksToRun, m_pendingCallbacks);
    }
    while (!callbacksToRun.empty()) {
        auto callback = callbacksToRun.front();
        callbacksToRun.pop();
        callback();
    }
}

GlobalHotkeyManager::~GlobalHotkeyManager()
{
    hook_stop();
}

struct GlobalHotkeyManager::Hotkey {
    uint16_t keycode;
    uint32_t modifiers;
    HotkeyCallback callback;
};

int GlobalHotkeyManager::registerHotkey(uint16_t keycode, uint32_t modifiers, HotkeyCallback callback) {
    std::lock_guard<std::mutex> lock(m_mutex);
    int id = m_nextId++;
    m_callbacks[id] = {
        keycode,
        modifiers,
        callback
    };
    
    return id;
}

GlobalHotkeyManager::Hotkey* GlobalHotkeyManager::find_hotkey(uint16_t keycode, uint32_t modifiers) {
    std::lock_guard<std::mutex> lock(m_mutex);
    auto it = std::find_if(m_callbacks.begin(), m_callbacks.end(),
        [keycode, modifiers](const auto& pair) {
            return pair.second.keycode == keycode && (modifiers & pair.second.modifiers);
        });

    if (it != m_callbacks.end()) {
        return &it->second;
    }

    return nullptr;
}

void GlobalHotkeyManager::unregisterHotkey(int id) {
    std::lock_guard<std::mutex> lock(m_mutex);
    m_callbacks.erase(id);
}

void GlobalHotkeyManager::hook_callback(uiohook_event* const event) {
    GlobalHotkeyManager::instance().handleEvent(event);
}

void GlobalHotkeyManager::handleEvent(uiohook_event* const event) {
    if (event->type == EVENT_KEY_PRESSED) {
        Hotkey *hk = find_hotkey(event->data.keyboard.keycode, event->mask);
        if (hk != nullptr) {
            {
                std::lock_guard<std::mutex> lock(m_queueMutex);
                m_pendingCallbacks.push(hk->callback);
            }
            m_dispatcher.emit();
        }
    }
}
#endif