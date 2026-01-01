#ifdef WITH_HOTKEYS
#include "sigc++/functors/mem_fun.h"
#include "GlobalHotkeyManager.hpp"
#include "spdlog/spdlog.h"
#include "gdk/gdkkeysyms.h"
#include "uiohook.h"

// Linker will complain otherwise
GlobalHotkeyManager* GlobalHotkeyManager::s_instance = nullptr;

GlobalHotkeyManager::GlobalHotkeyManager() : m_nextId(1) {
    m_dispatcher.connect(sigc::mem_fun(*this, &GlobalHotkeyManager::processCallbacks));

    s_instance = this;
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

struct GlobalHotkeyManager::Hotkey {
    uint16_t keycode;
    uint32_t modifiers;
    HotkeyCallback callback;
};

GlobalHotkeyManager::~GlobalHotkeyManager()
{   
    // hook_stop() stop event processing
    // but I will leave this to prevent dangling references
    for (std::pair<const int, GlobalHotkeyManager::Hotkey> callback : m_callbacks) {
        unregisterHotkey(callback.first);
    }
    hook_stop();
    s_instance = nullptr;
}

int GlobalHotkeyManager::registerHotkey(uint16_t keycode, uint32_t modifiers, HotkeyCallback callback) {
    if (find_hotkey(keycode, modifiers) != nullptr) {
        spdlog::get("ui")->error("Unable to register hotkey: Keycode {} with mask {} already in use.", keycode, modifiers);
        return -1;
    }

    std::lock_guard<std::mutex> lock(m_mutex);

    int id = m_nextId++;
    m_callbacks[id] = {
        keycode,
        modifiers,
        callback
    };
    
    return id;
}

int GlobalHotkeyManager::registerHotkey(const char *shortcut_str, HotkeyCallback callback) {
    auto parse_result = parse_and_convert_shortcut(shortcut_str);

    if (!parse_result.has_value()) {
        return -1;
    }

    auto values = parse_result.value();
    uint16_t keycode = std::get<0>(values);
    uint32_t modifiers = std::get<1>(values);
 
    return registerHotkey(keycode, modifiers, callback);
}

GlobalHotkeyManager::Hotkey* GlobalHotkeyManager::find_hotkey(uint16_t keycode, uint32_t modifiers) {
    // FIXME: When using multiple modifiers it does not care if just one is pressed or both
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
    if (s_instance) {
        s_instance->handleEvent(event);
    }
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

std::optional<std::tuple<uint16_t, uint32_t>> GlobalHotkeyManager::parse_and_convert_shortcut(const char *shortcut_str) {
    guint gdk_key = 0;
    GdkModifierType gdk_mods;

    gtk_accelerator_parse(shortcut_str, &gdk_key, &gdk_mods);

    if (gdk_key == 0) {
        spdlog::get("ui")->warn("Failed to parse shortcut: {}", shortcut_str);
        return std::nullopt;
    }

    uint16_t key  = convert_gdk_keyval_to_uihooks_key(gdk_key);
    uint32_t mods = convert_gdk_modifiers_to_uihooks(gdk_mods);

    if (key == VC_UNDEFINED) {
        spdlog::get("ui")->warn("Failed to parse key code: {}", gdk_key);
        return std::nullopt;
    }

    return std::make_tuple(key, mods);
}

uint32_t GlobalHotkeyManager::convert_gdk_modifiers_to_uihooks(GdkModifierType mods) {
    uint16_t hook_modifiers = 0;

    if (mods & GDK_SHIFT_MASK)   hook_modifiers |= MASK_SHIFT;
    if (mods & GDK_CONTROL_MASK) hook_modifiers |= MASK_CTRL;
    if (mods & GDK_MOD1_MASK)    hook_modifiers |= MASK_ALT;
    if (mods & GDK_META_MASK)    hook_modifiers |= MASK_META;
    // I don't think more masks would be necesary

    return hook_modifiers;
}

uint16_t GlobalHotkeyManager::convert_gdk_keyval_to_uihooks_key(guint keyval) {
    switch (keyval) {
        // Function Keys
        case GDK_KEY_F1:                return VC_F1;
        case GDK_KEY_F2:                return VC_F2;
        case GDK_KEY_F3:                return VC_F3;
        case GDK_KEY_F4:                return VC_F4;
        case GDK_KEY_F5:                return VC_F5;
        case GDK_KEY_F6:                return VC_F6;
        case GDK_KEY_F7:                return VC_F7;
        case GDK_KEY_F8:                return VC_F8;
        case GDK_KEY_F9:                return VC_F9;
        case GDK_KEY_F10:               return VC_F10;
        case GDK_KEY_F11:               return VC_F11;
        case GDK_KEY_F12:               return VC_F12;
        case GDK_KEY_F13:               return VC_F13;
        case GDK_KEY_F14:               return VC_F14;
        case GDK_KEY_F15:               return VC_F15;
        case GDK_KEY_F16:               return VC_F16;
        case GDK_KEY_F17:               return VC_F17;
        case GDK_KEY_F18:               return VC_F18;
        case GDK_KEY_F19:               return VC_F19;
        case GDK_KEY_F20:               return VC_F20;
        case GDK_KEY_F21:               return VC_F21;
        case GDK_KEY_F22:               return VC_F22;
        case GDK_KEY_F23:               return VC_F23;
        case GDK_KEY_F24:               return VC_F24;

        // Alphanumeric
        case GDK_KEY_grave:             return VC_BACKQUOTE;
        case GDK_KEY_0:                 return VC_0;
        case GDK_KEY_1:                 return VC_1;
        case GDK_KEY_2:                 return VC_2;
        case GDK_KEY_3:                 return VC_3;
        case GDK_KEY_4:                 return VC_4;
        case GDK_KEY_5:                 return VC_5;
        case GDK_KEY_6:                 return VC_6;
        case GDK_KEY_7:                 return VC_7;
        case GDK_KEY_8:                 return VC_8;
        case GDK_KEY_9:                 return VC_9;

        case GDK_KEY_minus:             return VC_MINUS;
        case GDK_KEY_equal:             return VC_EQUALS;
        case GDK_KEY_BackSpace:         return VC_BACKSPACE;

        case GDK_KEY_Tab:               return VC_TAB;
        case GDK_KEY_Caps_Lock:         return VC_CAPS_LOCK;

        case GDK_KEY_bracketleft:       return VC_OPEN_BRACKET;
        case GDK_KEY_bracketright:      return VC_CLOSE_BRACKET;
        case GDK_KEY_backslash:         return VC_BACK_SLASH;

        case GDK_KEY_semicolon:         return VC_SEMICOLON;
        case GDK_KEY_apostrophe:        return VC_QUOTE;
        case GDK_KEY_Return:            return VC_ENTER;
        
        case GDK_KEY_comma:             return VC_COMMA;
        case GDK_KEY_period:            return VC_PERIOD;
        case GDK_KEY_slash:             return VC_SLASH;
        
        case GDK_KEY_space:             return VC_SPACE;
        
        case GDK_KEY_A: case GDK_KEY_a: return VC_A;
        case GDK_KEY_B: case GDK_KEY_b: return VC_B;
        case GDK_KEY_C: case GDK_KEY_c: return VC_C;
        case GDK_KEY_D: case GDK_KEY_d: return VC_D;
        case GDK_KEY_E: case GDK_KEY_e: return VC_E;
        case GDK_KEY_F: case GDK_KEY_f: return VC_F;
        case GDK_KEY_G: case GDK_KEY_g: return VC_G;
        case GDK_KEY_H: case GDK_KEY_h: return VC_H;
        case GDK_KEY_I: case GDK_KEY_i: return VC_I;
        case GDK_KEY_J: case GDK_KEY_j: return VC_J;
        case GDK_KEY_K: case GDK_KEY_k: return VC_K;
        case GDK_KEY_L: case GDK_KEY_l: return VC_L;
        case GDK_KEY_M: case GDK_KEY_m: return VC_M;
        case GDK_KEY_N: case GDK_KEY_n: return VC_N;
        case GDK_KEY_O: case GDK_KEY_o: return VC_O;
        case GDK_KEY_P: case GDK_KEY_p: return VC_P;
        case GDK_KEY_Q: case GDK_KEY_q: return VC_Q;
        case GDK_KEY_R: case GDK_KEY_r: return VC_R;
        case GDK_KEY_S: case GDK_KEY_s: return VC_S;
        case GDK_KEY_T: case GDK_KEY_t: return VC_T;
        case GDK_KEY_U: case GDK_KEY_u: return VC_U;
        case GDK_KEY_V: case GDK_KEY_v: return VC_V;
        case GDK_KEY_W: case GDK_KEY_w: return VC_W;
        case GDK_KEY_X: case GDK_KEY_x: return VC_X;
        case GDK_KEY_Y: case GDK_KEY_y: return VC_Y;
        case GDK_KEY_Z: case GDK_KEY_z: return VC_Z;

        case GDK_KEY_Print:             return VC_PRINTSCREEN;
        case GDK_KEY_Scroll_Lock:       return VC_SCROLL_LOCK;
        case GDK_KEY_Pause:             return VC_PAUSE;

        case GDK_KEY_less:              return VC_LESSER_GREATER;

        // Edit Key
        case GDK_KEY_Insert:            return VC_INSERT;
        case GDK_KEY_Delete:            return VC_DELETE;
        case GDK_KEY_Home:              return VC_HOME;
        case GDK_KEY_End:               return VC_END;
        case GDK_KEY_Page_Up:           return VC_PAGE_UP;
        case GDK_KEY_Page_Down:         return VC_PAGE_DOWN;
        
        // Cursor Key
        case GDK_KEY_Up:                return VC_UP;
        case GDK_KEY_Left:              return VC_LEFT;
        case GDK_KEY_Clear:             return VC_CLEAR;
        case GDK_KEY_Right:             return VC_RIGHT;
        case GDK_KEY_Down:              return VC_DOWN;

        // Numeric
        case GDK_KEY_Num_Lock:          return VC_NUM_LOCK;
        case GDK_KEY_KP_Divide:         return VC_KP_DIVIDE;
        case GDK_KEY_KP_Multiply:       return VC_KP_MULTIPLY;
        case GDK_KEY_KP_Subtract:       return VC_KP_SUBTRACT;
        case GDK_KEY_KP_Equal:          return VC_KP_EQUALS;
        case GDK_KEY_KP_Add:            return VC_KP_ADD;
        case GDK_KEY_KP_Enter:          return VC_KP_ENTER;
        case GDK_KEY_KP_Separator:      return VC_KP_SEPARATOR;
        
        case GDK_KEY_KP_0:              return VC_KP_0;
        case GDK_KEY_KP_1:              return VC_KP_1;
        case GDK_KEY_KP_2:              return VC_KP_2;
        case GDK_KEY_KP_3:              return VC_KP_3;
        case GDK_KEY_KP_4:              return VC_KP_4;
        case GDK_KEY_KP_5:              return VC_KP_5;
        case GDK_KEY_KP_6:              return VC_KP_6;
        case GDK_KEY_KP_7:              return VC_KP_7;
        case GDK_KEY_KP_8:              return VC_KP_8;
        case GDK_KEY_KP_9:              return VC_KP_9;
        
        case GDK_KEY_KP_End:            return VC_KP_END;
        case GDK_KEY_KP_Down:           return VC_KP_DOWN;
        case GDK_KEY_KP_Page_Down:      return VC_KP_PAGE_DOWN;
        case GDK_KEY_KP_Left:           return VC_KP_LEFT;
        case GDK_KEY_KP_Begin:          return VC_KP_CLEAR;
        case GDK_KEY_KP_Right:          return VC_KP_RIGHT;
        case GDK_KEY_KP_Home:           return VC_KP_HOME;
        case GDK_KEY_KP_Up:             return VC_KP_UP;
        case GDK_KEY_KP_Page_Up:        return VC_KP_PAGE_UP;
        case GDK_KEY_KP_Insert:         return VC_KP_INSERT;
        case GDK_KEY_KP_Delete:         return VC_KP_DELETE;
        
        // Modifier and Control Keys
        case GDK_KEY_Shift_L:           return VC_SHIFT_L;
        case GDK_KEY_Shift_R:           return VC_SHIFT_R;
        case GDK_KEY_Control_L:         return VC_CONTROL_L;
        case GDK_KEY_Control_R:         return VC_CONTROL_R;
        case GDK_KEY_Alt_L:             return VC_ALT_L;
        case GDK_KEY_Alt_R:             return VC_ALT_R;
        case GDK_KEY_Meta_L:            return VC_META_L;
        case GDK_KEY_Meta_R:            return VC_META_R;
        case GDK_KEY_Menu:              return VC_CONTEXT_MENU;

        // Media Control Keys
        case GDK_KEY_AudioPlay:         return VC_MEDIA_PLAY;
        case GDK_KEY_AudioStop:         return VC_MEDIA_STOP;
        case GDK_KEY_AudioPrev:         return VC_MEDIA_PREVIOUS;
        case GDK_KEY_AudioNext:         return VC_MEDIA_NEXT;

        default:                        return VC_UNDEFINED;
    }
}
#endif