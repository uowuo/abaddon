#pragma once
#include <gtkmm.h>
#include <string>
#include <queue>
#include <mutex>
#include <unordered_set>
#include <unordered_map>
#include <sigc++/sigc++.h>
#include "../discord/discord.hpp"

constexpr static int GuildIconSize = 24;
constexpr static int DMIconSize = 20;
constexpr static int OrphanChannelSortOffset = -100; // forces orphan channels to the top of the list

enum class RenderType {
    Guild,
    Category,
    TextChannel,

    DMHeader,
    DM,
};

class CellRendererChannels : public Gtk::CellRenderer {
public:
    CellRendererChannels();
    virtual ~CellRendererChannels();

    Glib::PropertyProxy<RenderType> property_type();
    Glib::PropertyProxy<Glib::ustring> property_name();
    Glib::PropertyProxy<Glib::RefPtr<Gdk::Pixbuf>> property_icon();
    Glib::PropertyProxy<bool> property_expanded();

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

private:
    Gtk::CellRendererText m_renderer_text;

    Glib::Property<RenderType> m_property_type;

    Glib::Property<Glib::ustring> m_property_name;               // guild
    Glib::Property<Glib::RefPtr<Gdk::Pixbuf>> m_property_pixbuf; // guild
    Glib::Property<bool> m_property_expanded;                    // category
};

class ChannelList : public Gtk::ScrolledWindow {
public:
    ChannelList();
    void UpdateListing();
    void UpdateNewGuild(Snowflake id);
    void UpdateRemoveGuild(Snowflake id);
    void UpdateRemoveChannel(Snowflake id);
    void UpdateChannel(Snowflake id);
    void UpdateCreateDMChannel(Snowflake id);
    void UpdateCreateChannel(Snowflake id);
    void UpdateGuild(Snowflake id);

    void SetActiveChannel(Snowflake id);

protected:
    Gtk::TreeView m_view;

    class ModelColumns : public Gtk::TreeModel::ColumnRecord {
    public:
        ModelColumns();

        Gtk::TreeModelColumn<RenderType> m_type;
        Gtk::TreeModelColumn<uint64_t> m_id;
        Gtk::TreeModelColumn<Glib::ustring> m_name;
        Gtk::TreeModelColumn<Glib::RefPtr<Gdk::Pixbuf>> m_icon;
        Gtk::TreeModelColumn<int64_t> m_sort;
        // Gtk::CellRenderer's property_is_expanded only works how i want it to if it has children
        // because otherwise it doesnt count as an "expander" (property_is_expander)
        // so this solution will have to do which i hate but the alternative is adding invisible children
        // to all categories without children and having a filter model but that sounds worse
        // of course its a lot better than the absolute travesty i had before
        Gtk::TreeModelColumn<bool> m_expanded;
    };

    ModelColumns m_columns;
    Glib::RefPtr<Gtk::TreeStore> m_model;

    Gtk::TreeModel::iterator AddGuild(const GuildData &guild);
    Gtk::TreeModel::iterator UpdateCreateChannelCategory(const ChannelData &channel);

    void UpdateChannelCategory(const ChannelData &channel);

    // separation necessary because a channel and guild can share the same id
    Gtk::TreeModel::iterator GetIteratorForGuildFromID(Snowflake id);
    Gtk::TreeModel::iterator GetIteratorForChannelFromID(Snowflake id);

    bool IsTextChannel(ChannelType type);

    void OnRowCollapsed(const Gtk::TreeModel::iterator &iter, const Gtk::TreeModel::Path &path);
    void OnRowExpanded(const Gtk::TreeModel::iterator &iter, const Gtk::TreeModel::Path &path);
    bool SelectionFunc(const Glib::RefPtr<Gtk::TreeModel> &model, const Gtk::TreeModel::Path &path, bool is_currently_selected);

    Gtk::TreeModel::Path m_last_selected;
    Gtk::TreeModel::Path m_dm_header;

    void AddPrivateChannels();
    void UpdateCreateDMChannel(const ChannelData &channel);

    void OnMessageCreate(const Message &msg);

public:
    typedef sigc::signal<void, Snowflake> type_signal_action_channel_item_select;
    typedef sigc::signal<void, Snowflake> type_signal_action_guild_leave;
    typedef sigc::signal<void, Snowflake> type_signal_action_guild_settings;

    type_signal_action_channel_item_select signal_action_channel_item_select();
    type_signal_action_guild_leave signal_action_guild_leave();
    type_signal_action_guild_settings signal_action_guild_settings();

protected:
    type_signal_action_channel_item_select m_signal_action_channel_item_select;
    type_signal_action_guild_leave m_signal_action_guild_leave;
    type_signal_action_guild_settings m_signal_action_guild_settings;
};
