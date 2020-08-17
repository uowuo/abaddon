#include <gtkmm.h>
#include "discord/discord.hpp"

class Abaddon {
public:
    int DoMainLoop();
    void StartDiscordThread();

    void ActionConnect();

private:
    Glib::RefPtr<Gtk::Application> m_gtk_app;
    DiscordClient m_discord;
};