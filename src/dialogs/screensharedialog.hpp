#pragma once
#ifdef WITH_VIDEO

#include <gtkmm/dialog.h>
#include <gtkmm/notebook.h>
#include <gtkmm/box.h>
#include <gtkmm/button.h>
#include <gtkmm/label.h>
#include <gtkmm/scrolledwindow.h>
#include <optional>
#include <vector>

struct ScreenSource {
    std::string name;
    int x, y, width, height; // Geometría para pasar a FFmpeg
    bool is_window;          // true = ventana, false = monitor completo
    unsigned long xid;       // ID de ventana X11 (si aplica, para futuro)

    ScreenSource() : x(0), y(0), width(0), height(0), is_window(false), xid(0) {}
};

class ScreenShareDialog : public Gtk::Dialog {
public:
    ScreenShareDialog(Gtk::Window& parent);
    std::optional<ScreenSource> get_selected_source() const;

private:
    Gtk::Notebook m_notebook;
    Gtk::Box m_screens_box;
    // Gtk::Box m_windows_box; // Implementar luego con libwnck

    std::optional<ScreenSource> m_selected;

    void load_monitors();
    void on_source_clicked(ScreenSource source);
};

#endif
