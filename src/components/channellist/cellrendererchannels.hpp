#pragma once
#include <map>
#include <optional>
#include <gdkmm/pixbufanimation.h>
#include <glibmm/property.h>
#include <gtkmm/cellrendererpixbuf.h>
#include <gtkmm/cellrenderertext.h>
#include "discord/snowflake.hpp"
#include "discord/voicestateflags.hpp"
#include "misc/bitwise.hpp"

enum class RenderType : uint8_t {
    Folder,
    Guild,
    Category,
    TextChannel,
    Thread,

// TODO: maybe enable anyways but without ability to join if no voice support
#ifdef WITH_VOICE
    VoiceChannel,
    VoiceParticipant,
#endif

    DMHeader,
    DM,
};

class CellRendererChannels : public Gtk::CellRenderer {
public:
    CellRendererChannels();
    ~CellRendererChannels() override = default;

    Glib::PropertyProxy<RenderType> property_type();
    Glib::PropertyProxy<uint64_t> property_id();
    Glib::PropertyProxy<Glib::ustring> property_name();
    Glib::PropertyProxy<Glib::RefPtr<Gdk::Pixbuf>> property_icon();
    Glib::PropertyProxy<Glib::RefPtr<Gdk::PixbufAnimation>> property_icon_animation();
    Glib::PropertyProxy<bool> property_expanded();
    Glib::PropertyProxy<bool> property_nsfw();
    Glib::PropertyProxy<std::optional<Gdk::RGBA>> property_color();
    Glib::PropertyProxy<VoiceStateFlags> property_voice_state();

protected:
    void get_preferred_width_vfunc(Gtk::Widget &widget, int &minimum_width, int &natural_width) const override;
    void get_preferred_width_for_height_vfunc(Gtk::Widget &widget, int height, int &minimum_width, int &natural_width) const override;
    void get_preferred_height_vfunc(Gtk::Widget &widget, int &minimum_height, int &natural_height) const override;
    void get_preferred_height_for_width_vfunc(Gtk::Widget &widget, int width, int &minimum_height, int &natural_height) const override;
    void render_vfunc(const Cairo::RefPtr<Cairo::Context> &cr,
                      Gtk::Widget &widget,
                      const Gdk::Rectangle &background_area,
                      const Gdk::Rectangle &cell_area,
                      Gtk::CellRendererState flags) override;

    // guild functions
    void get_preferred_width_vfunc_folder(Gtk::Widget &widget, int &minimum_width, int &natural_width) const;
    void get_preferred_width_for_height_vfunc_folder(Gtk::Widget &widget, int height, int &minimum_width, int &natural_width) const;
    void get_preferred_height_vfunc_folder(Gtk::Widget &widget, int &minimum_height, int &natural_height) const;
    void get_preferred_height_for_width_vfunc_folder(Gtk::Widget &widget, int width, int &minimum_height, int &natural_height) const;
    void render_vfunc_folder(const Cairo::RefPtr<Cairo::Context> &cr,
                             Gtk::Widget &widget,
                             const Gdk::Rectangle &background_area,
                             const Gdk::Rectangle &cell_area,
                             Gtk::CellRendererState flags);

    // guild functions
    void get_preferred_width_vfunc_guild(Gtk::Widget &widget, int &minimum_width, int &natural_width) const;
    void get_preferred_width_for_height_vfunc_guild(Gtk::Widget &widget, int height, int &minimum_width, int &natural_width) const;
    void get_preferred_height_vfunc_guild(Gtk::Widget &widget, int &minimum_height, int &natural_height) const;
    void get_preferred_height_for_width_vfunc_guild(Gtk::Widget &widget, int width, int &minimum_height, int &natural_height) const;
    void render_vfunc_guild(const Cairo::RefPtr<Cairo::Context> &cr,
                            Gtk::Widget &widget,
                            const Gdk::Rectangle &background_area,
                            const Gdk::Rectangle &cell_area,
                            Gtk::CellRendererState flags);

    // category
    void get_preferred_width_vfunc_category(Gtk::Widget &widget, int &minimum_width, int &natural_width) const;
    void get_preferred_width_for_height_vfunc_category(Gtk::Widget &widget, int height, int &minimum_width, int &natural_width) const;
    void get_preferred_height_vfunc_category(Gtk::Widget &widget, int &minimum_height, int &natural_height) const;
    void get_preferred_height_for_width_vfunc_category(Gtk::Widget &widget, int width, int &minimum_height, int &natural_height) const;
    void render_vfunc_category(const Cairo::RefPtr<Cairo::Context> &cr,
                               Gtk::Widget &widget,
                               const Gdk::Rectangle &background_area,
                               const Gdk::Rectangle &cell_area,
                               Gtk::CellRendererState flags);

    // text channel
    void get_preferred_width_vfunc_channel(Gtk::Widget &widget, int &minimum_width, int &natural_width) const;
    void get_preferred_width_for_height_vfunc_channel(Gtk::Widget &widget, int height, int &minimum_width, int &natural_width) const;
    void get_preferred_height_vfunc_channel(Gtk::Widget &widget, int &minimum_height, int &natural_height) const;
    void get_preferred_height_for_width_vfunc_channel(Gtk::Widget &widget, int width, int &minimum_height, int &natural_height) const;
    void render_vfunc_channel(const Cairo::RefPtr<Cairo::Context> &cr,
                              Gtk::Widget &widget,
                              const Gdk::Rectangle &background_area,
                              const Gdk::Rectangle &cell_area,
                              Gtk::CellRendererState flags);

    // thread
    void get_preferred_width_vfunc_thread(Gtk::Widget &widget, int &minimum_width, int &natural_width) const;
    void get_preferred_width_for_height_vfunc_thread(Gtk::Widget &widget, int height, int &minimum_width, int &natural_width) const;
    void get_preferred_height_vfunc_thread(Gtk::Widget &widget, int &minimum_height, int &natural_height) const;
    void get_preferred_height_for_width_vfunc_thread(Gtk::Widget &widget, int width, int &minimum_height, int &natural_height) const;
    void render_vfunc_thread(const Cairo::RefPtr<Cairo::Context> &cr,
                             Gtk::Widget &widget,
                             const Gdk::Rectangle &background_area,
                             const Gdk::Rectangle &cell_area,
                             Gtk::CellRendererState flags);

#ifdef WITH_VOICE
    // voice channel
    void get_preferred_width_vfunc_voice_channel(Gtk::Widget &widget, int &minimum_width, int &natural_width) const;
    void get_preferred_width_for_height_vfunc_voice_channel(Gtk::Widget &widget, int height, int &minimum_width, int &natural_width) const;
    void get_preferred_height_vfunc_voice_channel(Gtk::Widget &widget, int &minimum_height, int &natural_height) const;
    void get_preferred_height_for_width_vfunc_voice_channel(Gtk::Widget &widget, int width, int &minimum_height, int &natural_height) const;
    void render_vfunc_voice_channel(const Cairo::RefPtr<Cairo::Context> &cr,
                                    Gtk::Widget &widget,
                                    const Gdk::Rectangle &background_area,
                                    const Gdk::Rectangle &cell_area,
                                    Gtk::CellRendererState flags);

    // voice participant
    void get_preferred_width_vfunc_voice_participant(Gtk::Widget &widget, int &minimum_width, int &natural_width) const;
    void get_preferred_width_for_height_vfunc_voice_participant(Gtk::Widget &widget, int height, int &minimum_width, int &natural_width) const;
    void get_preferred_height_vfunc_voice_participant(Gtk::Widget &widget, int &minimum_height, int &natural_height) const;
    void get_preferred_height_for_width_vfunc_voice_participant(Gtk::Widget &widget, int width, int &minimum_height, int &natural_height) const;
    void render_vfunc_voice_participant(const Cairo::RefPtr<Cairo::Context> &cr,
                                        Gtk::Widget &widget,
                                        const Gdk::Rectangle &background_area,
                                        const Gdk::Rectangle &cell_area,
                                        Gtk::CellRendererState flags);
#endif

    // dm header
    void get_preferred_width_vfunc_dmheader(Gtk::Widget &widget, int &minimum_width, int &natural_width) const;
    void get_preferred_width_for_height_vfunc_dmheader(Gtk::Widget &widget, int height, int &minimum_width, int &natural_width) const;
    void get_preferred_height_vfunc_dmheader(Gtk::Widget &widget, int &minimum_height, int &natural_height) const;
    void get_preferred_height_for_width_vfunc_dmheader(Gtk::Widget &widget, int width, int &minimum_height, int &natural_height) const;
    void render_vfunc_dmheader(const Cairo::RefPtr<Cairo::Context> &cr,
                               Gtk::Widget &widget,
                               const Gdk::Rectangle &background_area,
                               const Gdk::Rectangle &cell_area,
                               Gtk::CellRendererState flags);

    // dm
    void get_preferred_width_vfunc_dm(Gtk::Widget &widget, int &minimum_width, int &natural_width) const;
    void get_preferred_width_for_height_vfunc_dm(Gtk::Widget &widget, int height, int &minimum_width, int &natural_width) const;
    void get_preferred_height_vfunc_dm(Gtk::Widget &widget, int &minimum_height, int &natural_height) const;
    void get_preferred_height_for_width_vfunc_dm(Gtk::Widget &widget, int width, int &minimum_height, int &natural_height) const;
    void render_vfunc_dm(const Cairo::RefPtr<Cairo::Context> &cr,
                         Gtk::Widget &widget,
                         const Gdk::Rectangle &background_area,
                         const Gdk::Rectangle &cell_area,
                         Gtk::CellRendererState flags);

    static void cairo_path_rounded_rect(const Cairo::RefPtr<Cairo::Context> &cr, double x, double y, double w, double h, double r);
    static void unread_render_mentions(const Cairo::RefPtr<Cairo::Context> &cr, Gtk::Widget &widget, int mentions, int edge, const Gdk::Rectangle &cell_area);

private:
    Gtk::CellRendererText m_renderer_text;
    Gtk::CellRendererPixbuf m_renderer_pixbuf;

    Glib::Property<RenderType> m_property_type;    // all
    Glib::Property<Glib::ustring> m_property_name; // all
    Glib::Property<uint64_t> m_property_id;
    Glib::Property<Glib::RefPtr<Gdk::Pixbuf>> m_property_pixbuf;                    // guild, dm
    Glib::Property<Glib::RefPtr<Gdk::PixbufAnimation>> m_property_pixbuf_animation; // guild
    Glib::Property<bool> m_property_expanded;                                       // category
    Glib::Property<bool> m_property_nsfw;                                           // channel
    Glib::Property<std::optional<Gdk::RGBA>> m_property_color;                      // folder
    Glib::Property<VoiceStateFlags> m_property_voice_state;

    // same pitfalls as in https://github.com/uowuo/abaddon/blob/60404783bd4ce9be26233fe66fc3a74475d9eaa3/components/cellrendererpixbufanimation.hpp#L32-L39
    // this will manifest though since guild icons can change
    // an animation or two wont be the end of the world though
    std::map<Glib::RefPtr<Gdk::PixbufAnimation>, Glib::RefPtr<Gdk::PixbufAnimationIter>> m_pixbuf_anim_iters;
};
