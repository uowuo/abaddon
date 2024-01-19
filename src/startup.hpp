#pragma once

#include <optional>

#include <glibmm/dispatcher.h>
#include <gtkmm/messagedialog.h>

// fetch cookies, build number async

class DiscordStartupDialog : public Gtk::MessageDialog {
public:
    DiscordStartupDialog(Gtk::Window &window);

    [[nodiscard]] std::optional<std::string> GetCookie() const;
    [[nodiscard]] std::optional<uint32_t> GetBuildNumber() const;

private:
    void RunAsync();

    void DispatchCallback();

    Glib::Dispatcher m_dispatcher;

    std::optional<std::string> m_cookie;
    std::optional<uint32_t> m_build_number;
};
