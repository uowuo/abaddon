#ifdef WITH_VIDEO

#include "screensharedialog.hpp"
#include <gdkmm/display.h>
#include <gdkmm/monitor.h>
#include <spdlog/spdlog.h>

#ifdef _WIN32
#include <Windows.h>
#include <vector>
#endif

#ifdef __APPLE__
#include <CoreGraphics/CoreGraphics.h>
#include <vector>
#endif

ScreenShareDialog::ScreenShareDialog(Gtk::Window& parent)
    : Gtk::Dialog("Compartir Pantalla", parent, true)
    , m_screens_box(Gtk::ORIENTATION_VERTICAL) {
    set_default_size(400, 300);
    get_style_context()->add_class("app-window");
    get_style_context()->add_class("app-popup");

    // Pestaña Pantallas
    m_screens_box.set_spacing(10);
    m_screens_box.set_margin_start(10);
    m_screens_box.set_margin_end(10);
    m_screens_box.set_margin_top(10);
    m_screens_box.set_margin_bottom(10);
    m_notebook.append_page(m_screens_box, "Pantallas");

    // Pestaña Ventanas (placeholder para futuro)
    // m_windows_box.set_spacing(10);
    // m_notebook.append_page(m_windows_box, "Ventanas");

    load_monitors();

    add_button("Cancelar", Gtk::RESPONSE_CANCEL);

    get_content_area()->pack_start(m_notebook, true, true, 0);

    show_all_children();
}

void ScreenShareDialog::load_monitors() {
#ifdef __linux__
    auto display = Gdk::Display::get_default();
    if (!display) {
        if (auto logger = spdlog::get("ui")) {
            logger->error("Failed to get default display");
        }
        return;
    }

    int n_monitors = display->get_n_monitors();
    if (auto logger = spdlog::get("ui")) {
        logger->info("Found {} monitors", n_monitors);
    }

    for (int i = 0; i < n_monitors; i++) {
        auto monitor = display->get_monitor(i);
        if (!monitor) continue;

        Gdk::Rectangle geo;
        monitor->get_geometry(geo);

        ScreenSource src;
        src.name = "Monitor " + std::to_string(i + 1);
        src.x = geo.get_x();
        src.y = geo.get_y();
        src.width = geo.get_width();
        src.height = geo.get_height();
        src.is_window = false;

        // Crear un botón por monitor
        auto btn = Gtk::manage(new Gtk::Button());
        std::string label_text = src.name + "\n" +
                                std::to_string(src.width) + "x" + std::to_string(src.height) +
                                " @ (" + std::to_string(src.x) + ", " + std::to_string(src.y) + ")";
        btn->set_label(label_text);
        btn->set_halign(Gtk::ALIGN_FILL);
        btn->set_margin_bottom(5);

        btn->signal_clicked().connect([this, src]() {
            on_source_clicked(src);
        });

        m_screens_box.pack_start(*btn, false, false, 0);
    }

#elif defined(_WIN32)
    struct MonitorInfo {
        int x, y, width, height;
        std::string name;
    };
    std::vector<MonitorInfo> monitors;

    // Static callback function for EnumDisplayMonitors (must be static or global)
    static BOOL CALLBACK MonitorEnumProc(HMONITOR hMonitor, HDC hdcMonitor, LPRECT lprcMonitor, LPARAM dwData) {
        auto* monitors_vec = reinterpret_cast<std::vector<MonitorInfo>*>(dwData);
        MonitorInfo info;
        info.x = lprcMonitor->left;
        info.y = lprcMonitor->top;
        info.width = lprcMonitor->right - lprcMonitor->left;
        info.height = lprcMonitor->bottom - lprcMonitor->top;

        MONITORINFOEX monitorInfo;
        monitorInfo.cbSize = sizeof(MONITORINFOEX);
        if (GetMonitorInfo(hMonitor, &monitorInfo)) {
            if (sizeof(TCHAR) == 2) {
                // Wide build: convert UTF-16 to UTF-8
                int utf8_len = WideCharToMultiByte(CP_UTF8, 0, monitorInfo.szDevice, -1, nullptr, 0, nullptr, nullptr);
                if (utf8_len > 0) {
                    std::vector<char> utf8_buffer(utf8_len);
                    int result = WideCharToMultiByte(CP_UTF8, 0, monitorInfo.szDevice, -1, utf8_buffer.data(), utf8_len, nullptr, nullptr);
                    if (result > 0) {
                        info.name = std::string(utf8_buffer.data());
                    } else {
                        info.name = "Monitor " + std::to_string(monitors_vec->size() + 1);
                    }
                } else {
                    info.name = "Monitor " + std::to_string(monitors_vec->size() + 1);
                }
            } else {
                // ANSI build: direct assignment
                info.name = std::string(monitorInfo.szDevice);
            }
        } else {
            info.name = "Monitor " + std::to_string(monitors_vec->size() + 1);
        }

        monitors_vec->push_back(info);
        return TRUE;
    }

    EnumDisplayMonitors(NULL, NULL, MonitorEnumProc, reinterpret_cast<LPARAM>(&monitors));

    if (auto logger = spdlog::get("ui")) {
        logger->info("Found {} monitors on Windows", monitors.size());
    }

    for (size_t i = 0; i < monitors.size(); i++) {
        const auto& mon = monitors[i];

        ScreenSource src;
        src.name = "Monitor " + std::to_string(i + 1);
        src.x = mon.x;
        src.y = mon.y;
        src.width = mon.width;
        src.height = mon.height;
        src.is_window = false;

        auto btn = Gtk::manage(new Gtk::Button());
        std::string label_text = src.name + "\n" +
                                std::to_string(src.width) + "x" + std::to_string(src.height) +
                                " @ (" + std::to_string(src.x) + ", " + std::to_string(src.y) + ")";
        btn->set_label(label_text);
        btn->set_halign(Gtk::ALIGN_FILL);
        btn->set_margin_bottom(5);

        btn->signal_clicked().connect([this, src]() {
            on_source_clicked(src);
        });

        m_screens_box.pack_start(*btn, false, false, 0);
    }

#elif defined(__APPLE__)
    uint32_t display_count = 0;
    CGError err = CGGetActiveDisplayList(0, NULL, &display_count);
    if (err != kCGErrorSuccess) {
        if (auto logger = spdlog::get("ui")) {
            logger->error("Failed to get display count: {}", err);
        }
        return;
    }

    if (display_count == 0) {
        if (auto logger = spdlog::get("ui")) {
            logger->warn("No displays found");
        }
        return;
    }

    std::vector<CGDirectDisplayID> displays(display_count);
    err = CGGetActiveDisplayList(display_count, displays.data(), &display_count);
    if (err != kCGErrorSuccess) {
        if (auto logger = spdlog::get("ui")) {
            logger->error("Failed to get display list: {}", err);
        }
        return;
    }

    if (auto logger = spdlog::get("ui")) {
        logger->info("Found {} displays on macOS", display_count);
    }

    for (uint32_t i = 0; i < display_count; i++) {
        CGRect bounds = CGDisplayBounds(displays[i]);

        ScreenSource src;
        src.name = "Pantalla " + std::to_string(i + 1);
        src.x = static_cast<int>(bounds.origin.x);
        src.y = static_cast<int>(bounds.origin.y);
        src.width = static_cast<int>(bounds.size.width);
        src.height = static_cast<int>(bounds.size.height);
        src.is_window = false;

        auto btn = Gtk::manage(new Gtk::Button());
        std::string label_text = src.name + "\n" +
                                std::to_string(src.width) + "x" + std::to_string(src.height) +
                                " @ (" + std::to_string(src.x) + ", " + std::to_string(src.y) + ")";
        btn->set_label(label_text);
        btn->set_halign(Gtk::ALIGN_FILL);
        btn->set_margin_bottom(5);

        btn->signal_clicked().connect([this, src]() {
            on_source_clicked(src);
        });

        m_screens_box.pack_start(*btn, false, false, 0);
    }

#else
    if (auto logger = spdlog::get("ui")) {
        logger->warn("Platform not supported for monitor detection");
    }
#endif
}

void ScreenShareDialog::on_source_clicked(ScreenSource source) {
    m_selected = source;
    if (auto logger = spdlog::get("ui")) {
        logger->info("Selected screen source: {} ({}x{} @ {},{})",
                    source.name, source.width, source.height, source.x, source.y);
    }
    response(Gtk::RESPONSE_OK);
}

std::optional<ScreenSource> ScreenShareDialog::get_selected_source() const {
    return m_selected;
}

#endif
