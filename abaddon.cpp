#include <gtkmm.h>
#include "discord/discord.hpp"
#include "windows/mainwindow.hpp"
#include <memory>
#include "abaddon.hpp"

#ifdef _WIN32
    #pragma comment(lib, "crypt32.lib")
#endif

int Abaddon::DoMainLoop() {
    m_gtk_app = Gtk::Application::create("com.github.lorpus.abaddon");

    MainWindow main;
    main.SetAbaddon(this);
    main.set_title("Abaddon");
    main.show();

    m_gtk_app->signal_shutdown().connect([&]() {
        m_discord.Stop();
    });

    /*sigc::connection draw_signal_handler = main.signal_draw().connect([&](const Cairo::RefPtr<Cairo::Context> &ctx) -> bool {
            draw_signal_handler.disconnect();

            return false;
        });*/

    return m_gtk_app->run(main);
}

void Abaddon::StartDiscordThread() {
    m_discord.Start();
}

void Abaddon::ActionConnect() {
    if (!m_discord.IsStarted())
        StartDiscordThread();
}

int main(int argc, char **argv) {
    Abaddon abaddon;
    return abaddon.DoMainLoop();
}
