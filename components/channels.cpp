#include "channels.hpp"
#include <algorithm>
#include <map>
#include <unordered_map>
#include "../abaddon.hpp"
#include "../imgmanager.hpp"
#include "../util.hpp"
#include "statusindicator.hpp"

ChannelList::ChannelList()
    : m_model(Gtk::TreeStore::create(m_columns))
    , m_main(Gtk::manage(new Gtk::ScrolledWindow)) {
    const auto cb = [this](const Gtk::TreeModel::Path &path, Gtk::TreeViewColumn *column) {
        auto row = *m_model->get_iter(path);
        if (row[m_columns.m_expanded]) {
            m_view.collapse_row(path);
            row[m_columns.m_expanded] = false;
        } else {
            m_view.expand_row(path, false);
            row[m_columns.m_expanded] = true;
        }

        if (row[m_columns.m_type] == RenderType::TextChannel) {
            m_signal_action_channel_item_select.emit(static_cast<Snowflake>(row[m_columns.m_id]));
        }
    };
    m_view.signal_row_activated().connect(cb);
    m_view.set_activate_on_single_click(true);

    m_view.set_hexpand(true);
    m_view.set_vexpand(true);

    m_view.set_headers_visible(false);
    m_view.set_model(m_model);
    m_model->set_sort_column(m_columns.m_sort, Gtk::SORT_ASCENDING);

    m_view.show();

    m_main->add(m_view);
    m_main->show_all();

    auto *column = Gtk::manage(new Gtk::TreeView::Column("display"));
    auto *renderer = Gtk::manage(new CellRendererChannels);
    column->pack_start(*renderer);
    column->add_attribute(renderer->property_type(), m_columns.m_type);
    column->add_attribute(renderer->property_icon(), m_columns.m_icon);
    column->add_attribute(renderer->property_name(), m_columns.m_name);
    column->add_attribute(renderer->property_expanded(), m_columns.m_expanded);
    m_view.append_column(*column);
}

Gtk::Widget *ChannelList::GetRoot() const {
    return m_main;
}

void ChannelList::UpdateListing() {
    m_model->clear();

    auto &discord = Abaddon::Get().GetDiscordClient();
    auto &img = Abaddon::Get().GetImageManager();

    const auto guild_ids = discord.GetUserSortedGuilds();
    int sortnum = 0;
    for (const auto &guild_id : guild_ids) {
        const auto guild = discord.GetGuild(guild_id);
        if (!guild.has_value()) continue;

        auto iter = AddGuild(*guild);
        (*iter)[m_columns.m_sort] = sortnum++;
    }
}

void ChannelList::UpdateNewGuild(Snowflake id) {
    const auto guild = Abaddon::Get().GetDiscordClient().GetGuild(id);
    auto &img = Abaddon::Get().GetImageManager();

    if (!guild.has_value()) return;

    auto iter = AddGuild(*guild);

    // update sort order
    int sortnum = 0;
    for (const auto guild_id : Abaddon::Get().GetDiscordClient().GetUserSortedGuilds()) {
        auto iter = GetIteratorForGuildFromID(guild_id);
        if (iter)
            (*iter)[m_columns.m_sort] = ++sortnum;
    }
}

void ChannelList::UpdateRemoveGuild(Snowflake id) {
    auto iter = GetIteratorForGuildFromID(id);
    if (!iter) return;
    m_model->erase(iter);
}

void ChannelList::UpdateRemoveChannel(Snowflake id) {
    auto iter = GetIteratorForChannelFromID(id);
    if (!iter) return;
    m_model->erase(iter);
}

void ChannelList::UpdateChannel(Snowflake id) {
    auto iter = GetIteratorForChannelFromID(id);
    auto channel = Abaddon::Get().GetDiscordClient().GetChannel(id);
    if (!iter || !channel.has_value()) return;
    if (channel->Type == ChannelType::GUILD_CATEGORY) return UpdateChannelCategory(*channel);
    if (!IsTextChannel(channel->Type)) return;

    // delete and recreate
    m_model->erase(iter);

    Gtk::TreeStore::iterator parent;
    bool is_orphan;
    if (channel->ParentID.has_value()) {
        is_orphan = false;
        parent = GetIteratorForChannelFromID(*channel->ParentID);
    } else {
        is_orphan = true;
        parent = GetIteratorForGuildFromID(*channel->GuildID);
    }
    if (!parent) return;
    auto channel_row = *m_model->append(parent->children());
    channel_row[m_columns.m_type] = RenderType::TextChannel;
    channel_row[m_columns.m_id] = channel->ID;
    channel_row[m_columns.m_name] = Glib::Markup::escape_text(*channel->Name);
    if (is_orphan)
        channel_row[m_columns.m_sort] = *channel->Position + OrphanChannelSortOffset;
    else
        channel_row[m_columns.m_sort] = *channel->Position;
}

void ChannelList::UpdateCreateDMChannel(Snowflake id) {
}

void ChannelList::UpdateCreateChannel(Snowflake id) {
    const auto channel = Abaddon::Get().GetDiscordClient().GetChannel(id);
    if (!channel.has_value()) return;
    if (channel->Type == ChannelType::GUILD_CATEGORY) return (void)UpdateCreateChannelCategory(*channel);
    if (channel->Type != ChannelType::GUILD_TEXT && channel->Type != ChannelType::GUILD_NEWS) return;

    Gtk::TreeRow channel_row;
    bool orphan;
    if (channel->ParentID.has_value()) {
        orphan = false;
        auto iter = GetIteratorForChannelFromID(*channel->ParentID);
        channel_row = *m_model->append(iter->children());
    } else {
        orphan = true;
        auto iter = GetIteratorForGuildFromID(*channel->GuildID);
        channel_row = *m_model->append(iter->children());
    }
    channel_row[m_columns.m_type] = RenderType::TextChannel;
    channel_row[m_columns.m_id] = channel->ID;
    channel_row[m_columns.m_name] = Glib::Markup::escape_text(*channel->Name);
    if (orphan)
        channel_row[m_columns.m_sort] = *channel->Position + OrphanChannelSortOffset;
    else
        channel_row[m_columns.m_sort] = *channel->Position;
}

void ChannelList::UpdateGuild(Snowflake id) {
    auto iter = GetIteratorForGuildFromID(id);
    auto &img = Abaddon::Get().GetImageManager();
    const auto guild = Abaddon::Get().GetDiscordClient().GetGuild(id);
    if (!iter || !guild.has_value()) return;

    (*iter)[m_columns.m_name] = "<b>" + Glib::Markup::escape_text(guild->Name) + "</b>";
    (*iter)[m_columns.m_icon] = img.GetPlaceholder(GuildIconSize);
    if (guild->HasIcon()) {
        const auto cb = [this, id](const Glib::RefPtr<Gdk::Pixbuf> &pb) {
            // iter might be invalid
            auto iter = GetIteratorForGuildFromID(id);
            if (iter)
                (*iter)[m_columns.m_icon] = pb->scale_simple(GuildIconSize, GuildIconSize, Gdk::INTERP_BILINEAR);
        };
        img.LoadFromURL(guild->GetIconURL("png", "32"), sigc::track_obj(cb, *this));
    }
}

void ChannelList::SetActiveChannel(Snowflake id) {
}

Gtk::TreeModel::iterator ChannelList::AddGuild(const GuildData &guild) {
    auto &discord = Abaddon::Get().GetDiscordClient();
    auto &img = Abaddon::Get().GetImageManager();

    auto guild_row = *m_model->append();
    guild_row[m_columns.m_type] = RenderType::Guild;
    guild_row[m_columns.m_id] = guild.ID;
    guild_row[m_columns.m_name] = "<b>" + Glib::Markup::escape_text(guild.Name) + "</b>";
    guild_row[m_columns.m_icon] = img.GetPlaceholder(GuildIconSize);

    if (guild.HasIcon()) {
        const auto cb = [this, id = guild.ID](const Glib::RefPtr<Gdk::Pixbuf> &pb) {
            auto iter = GetIteratorForGuildFromID(id);
            if (iter)
                (*iter)[m_columns.m_icon] = pb->scale_simple(GuildIconSize, GuildIconSize, Gdk::INTERP_BILINEAR);
        };
        img.LoadFromURL(guild.GetIconURL("png", "32"), sigc::track_obj(cb, *this));
    }

    if (!guild.Channels.has_value()) return guild_row;

    // separate out the channels
    std::vector<ChannelData> orphan_channels;
    std::map<Snowflake, std::vector<ChannelData>> categories;

    for (const auto &channel_ : *guild.Channels) {
        const auto channel = discord.GetChannel(channel_.ID);
        if (!channel.has_value()) continue;
        if (channel->Type == ChannelType::GUILD_TEXT || channel->Type == ChannelType::GUILD_NEWS) {
            if (channel->ParentID.has_value())
                categories[*channel->ParentID].push_back(*channel);
            else
                orphan_channels.push_back(*channel);
        } else if (channel->Type == ChannelType::GUILD_CATEGORY) {
            categories[channel->ID];
        }
    }

    for (const auto &channel : orphan_channels) {
        auto channel_row = *m_model->append(guild_row.children());
        channel_row[m_columns.m_type] = RenderType::TextChannel;
        channel_row[m_columns.m_id] = channel.ID;
        channel_row[m_columns.m_name] = Glib::Markup::escape_text(*channel.Name);
        channel_row[m_columns.m_sort] = *channel.Position + OrphanChannelSortOffset;
    }

    for (const auto &[category_id, channels] : categories) {
        const auto category = discord.GetChannel(category_id);
        if (!category.has_value()) continue;
        auto cat_row = *m_model->append(guild_row.children());
        cat_row[m_columns.m_type] = RenderType::Category;
        cat_row[m_columns.m_id] = category_id;
        cat_row[m_columns.m_name] = Glib::Markup::escape_text(*category->Name);
        cat_row[m_columns.m_sort] = *category->Position;

        for (const auto &channel : channels) {
            auto channel_row = *m_model->append(cat_row.children());
            channel_row[m_columns.m_type] = RenderType::TextChannel;
            channel_row[m_columns.m_id] = channel.ID;
            channel_row[m_columns.m_name] = Glib::Markup::escape_text(*channel.Name);
            channel_row[m_columns.m_sort] = *channel.Position;
        }
    }

    return guild_row;
}

Gtk::TreeModel::iterator ChannelList::UpdateCreateChannelCategory(const ChannelData &channel) {
    const auto iter = GetIteratorForGuildFromID(*channel.GuildID);
    if (!iter) return {};

    auto cat_row = *m_model->append(iter->children());
    cat_row[m_columns.m_type] = RenderType::Category;
    cat_row[m_columns.m_id] = channel.ID;
    cat_row[m_columns.m_name] = Glib::Markup::escape_text(*channel.Name);
    cat_row[m_columns.m_sort] = *channel.Position;

    return cat_row;
}

void ChannelList::UpdateChannelCategory(const ChannelData &channel) {
    auto iter = GetIteratorForChannelFromID(channel.ID);
    if (!iter) return;

    (*iter)[m_columns.m_sort] = *channel.Position;
    (*iter)[m_columns.m_name] = Glib::Markup::escape_text(*channel.Name);
}

Gtk::TreeModel::iterator ChannelList::GetIteratorForGuildFromID(Snowflake id) {
    for (const auto child : m_model->children()) {
        if (child[m_columns.m_id] == id)
            return child;
    }
    return {};
}

Gtk::TreeModel::iterator ChannelList::GetIteratorForChannelFromID(Snowflake id) {
    std::queue<Gtk::TreeModel::iterator> queue;
    for (const auto child : m_model->children())
        for (const auto child2 : child.children())
            queue.push(child2);

    while (!queue.empty()) {
        auto item = queue.front();
        if ((*item)[m_columns.m_id] == id) return item;
        for (const auto child : item->children())
            queue.push(child);
        queue.pop();
    }

    return {};
}

bool ChannelList::IsTextChannel(ChannelType type) {
    return type == ChannelType::GUILD_TEXT || type == ChannelType::GUILD_NEWS;
}

ChannelList::type_signal_action_channel_item_select ChannelList::signal_action_channel_item_select() {
    return m_signal_action_channel_item_select;
}

ChannelList::type_signal_action_guild_leave ChannelList::signal_action_guild_leave() {
    return m_signal_action_guild_leave;
}

ChannelList::type_signal_action_guild_settings ChannelList::signal_action_guild_settings() {
    return m_signal_action_guild_settings;
}

ChannelList::ModelColumns::ModelColumns() {
    add(m_type);
    add(m_id);
    add(m_name);
    add(m_icon);
    add(m_sort);
    add(m_expanded);
}

CellRendererChannels::CellRendererChannels()
    : Glib::ObjectBase(typeid(CellRendererChannels))
    , Gtk::CellRenderer()
    , m_property_type(*this, "render-type")
    , m_property_name(*this, "name")
    , m_property_pixbuf(*this, "pixbuf")
    , m_property_expanded(*this, "expanded") {
    property_mode() = Gtk::CELL_RENDERER_MODE_ACTIVATABLE;
    property_xpad() = 2;
    property_ypad() = 2;
    m_property_name.get_proxy().signal_changed().connect([this] {
        m_renderer_text.property_markup() = m_property_name;
    });
}

CellRendererChannels::~CellRendererChannels() {
}

Glib::PropertyProxy<RenderType> CellRendererChannels::property_type() {
    return m_property_type.get_proxy();
}

Glib::PropertyProxy<Glib::ustring> CellRendererChannels::property_name() {
    return m_property_name.get_proxy();
}

Glib::PropertyProxy<Glib::RefPtr<Gdk::Pixbuf>> CellRendererChannels::property_icon() {
    return m_property_pixbuf.get_proxy();
}

Glib::PropertyProxy<bool> CellRendererChannels::property_expanded() {
    return m_property_expanded.get_proxy();
}

void CellRendererChannels::get_preferred_width_vfunc(Gtk::Widget &widget, int &minimum_width, int &natural_width) const {
    switch (m_property_type.get_value()) {
        case RenderType::Guild:
            return get_preferred_width_vfunc_guild(widget, minimum_width, natural_width);
        case RenderType::Category:
            return get_preferred_width_vfunc_category(widget, minimum_width, natural_width);
        case RenderType::TextChannel:
            return get_preferred_width_vfunc_channel(widget, minimum_width, natural_width);
    }
}

void CellRendererChannels::get_preferred_width_for_height_vfunc(Gtk::Widget &widget, int height, int &minimum_width, int &natural_width) const {
    switch (m_property_type.get_value()) {
        case RenderType::Guild:
            return get_preferred_width_for_height_vfunc_guild(widget, height, minimum_width, natural_width);
        case RenderType::Category:
            return get_preferred_width_for_height_vfunc_category(widget, height, minimum_width, natural_width);
        case RenderType::TextChannel:
            return get_preferred_width_for_height_vfunc_channel(widget, height, minimum_width, natural_width);
    }
}

void CellRendererChannels::get_preferred_height_vfunc(Gtk::Widget &widget, int &minimum_height, int &natural_height) const {
    switch (m_property_type.get_value()) {
        case RenderType::Guild:
            return get_preferred_height_vfunc_guild(widget, minimum_height, natural_height);
        case RenderType::Category:
            return get_preferred_height_vfunc_category(widget, minimum_height, natural_height);
        case RenderType::TextChannel:
            return get_preferred_height_vfunc_channel(widget, minimum_height, natural_height);
    }
}

void CellRendererChannels::get_preferred_height_for_width_vfunc(Gtk::Widget &widget, int width, int &minimum_height, int &natural_height) const {
    switch (m_property_type.get_value()) {
        case RenderType::Guild:
            return get_preferred_height_for_width_vfunc_guild(widget, width, minimum_height, natural_height);
        case RenderType::Category:
            return get_preferred_height_for_width_vfunc_category(widget, width, minimum_height, natural_height);
        case RenderType::TextChannel:
            return get_preferred_height_for_width_vfunc_channel(widget, width, minimum_height, natural_height);
    }
}

void CellRendererChannels::render_vfunc(const Cairo::RefPtr<Cairo::Context> &cr, Gtk::Widget &widget, const Gdk::Rectangle &background_area, const Gdk::Rectangle &cell_area, Gtk::CellRendererState flags) {
    switch (m_property_type.get_value()) {
        case RenderType::Guild:
            return render_vfunc_guild(cr, widget, background_area, cell_area, flags);
        case RenderType::Category:
            return render_vfunc_category(cr, widget, background_area, cell_area, flags);
        case RenderType::TextChannel:
            return render_vfunc_channel(cr, widget, background_area, cell_area, flags);
    }
}

// guild functions

void CellRendererChannels::get_preferred_width_vfunc_guild(Gtk::Widget &widget, int &minimum_width, int &natural_width) const {
    int pixbuf_width = 0;
    if (auto pixbuf = m_property_pixbuf.get_value())
        pixbuf_width = pixbuf->get_width();

    int text_min, text_nat;
    m_renderer_text.get_preferred_width(widget, text_min, text_nat);

    int xpad, ypad;
    get_padding(xpad, ypad);
    minimum_width = std::max(text_min, pixbuf_width) + xpad * 2;
    natural_width = std::max(text_nat, pixbuf_width) + xpad * 2;
}

void CellRendererChannels::get_preferred_width_for_height_vfunc_guild(Gtk::Widget &widget, int height, int &minimum_width, int &natural_width) const {
    get_preferred_width_vfunc_guild(widget, minimum_width, natural_width);
}

void CellRendererChannels::get_preferred_height_vfunc_guild(Gtk::Widget &widget, int &minimum_height, int &natural_height) const {
    int pixbuf_height = 0;
    if (auto pixbuf = m_property_pixbuf.get_value())
        pixbuf_height = pixbuf->get_height();

    int text_min, text_nat;
    m_renderer_text.get_preferred_height(widget, text_min, text_nat);

    int xpad, ypad;
    get_padding(xpad, ypad);
    minimum_height = std::max(text_min, pixbuf_height) + ypad * 2;
    natural_height = std::max(text_nat, pixbuf_height) + ypad * 2;
}

void CellRendererChannels::get_preferred_height_for_width_vfunc_guild(Gtk::Widget &widget, int width, int &minimum_height, int &natural_height) const {
    get_preferred_height_vfunc_guild(widget, minimum_height, natural_height);
}

void CellRendererChannels::render_vfunc_guild(const Cairo::RefPtr<Cairo::Context> &cr, Gtk::Widget &widget, const Gdk::Rectangle &background_area, const Gdk::Rectangle &cell_area, Gtk::CellRendererState flags) {
    Gtk::Requisition text_minimum, text_natural;
    m_renderer_text.get_preferred_size(widget, text_minimum, text_natural);

    Gtk::Requisition minimum, natural;
    get_preferred_size(widget, minimum, natural);

    auto pixbuf = m_property_pixbuf.get_value();

    const int icon_x = background_area.get_x();
    const int icon_y = background_area.get_y();
    const int icon_w = pixbuf->get_width();
    const int icon_h = pixbuf->get_height();

    const int text_x = icon_x + icon_w + 5;
    const int text_y = background_area.get_y() + background_area.get_height() / 2 - text_natural.height / 2;
    const int text_w = text_natural.width;
    const int text_h = text_natural.height;

    Gdk::Rectangle text_cell_area(text_x, text_y, text_w, text_h);

    m_renderer_text.render(cr, widget, background_area, text_cell_area, flags);

    Gdk::Cairo::set_source_pixbuf(cr, m_property_pixbuf.get_value(), icon_x, icon_y);
    cr->rectangle(icon_x, icon_y, icon_w, icon_h);
    cr->fill();
}

// category

void CellRendererChannels::get_preferred_width_vfunc_category(Gtk::Widget &widget, int &minimum_width, int &natural_width) const {
    m_renderer_text.get_preferred_width(widget, minimum_width, natural_width);
}

void CellRendererChannels::get_preferred_width_for_height_vfunc_category(Gtk::Widget &widget, int height, int &minimum_width, int &natural_width) const {
    m_renderer_text.get_preferred_width_for_height(widget, height, minimum_width, natural_width);
}

void CellRendererChannels::get_preferred_height_vfunc_category(Gtk::Widget &widget, int &minimum_height, int &natural_height) const {
    m_renderer_text.get_preferred_height(widget, minimum_height, natural_height);
}

void CellRendererChannels::get_preferred_height_for_width_vfunc_category(Gtk::Widget &widget, int width, int &minimum_height, int &natural_height) const {
    m_renderer_text.get_preferred_height_for_width(widget, width, minimum_height, natural_height);
}

void CellRendererChannels::render_vfunc_category(const Cairo::RefPtr<Cairo::Context> &cr, Gtk::Widget &widget, const Gdk::Rectangle &background_area, const Gdk::Rectangle &cell_area, Gtk::CellRendererState flags) {
    int available_xpad = background_area.get_width();
    int available_ypad = background_area.get_height();

    int x1, y1, x2, y2, x3, y3;
    if (property_expanded()) {
        x1 = background_area.get_x() + 7;
        y1 = background_area.get_y() + 5;
        x2 = background_area.get_x() + 12;
        y2 = background_area.get_y() + background_area.get_height() - 5;
        x3 = background_area.get_x() + 17;
        y3 = background_area.get_y() + 5;
    } else {
        x1 = background_area.get_x() + 7;
        y1 = background_area.get_y() + 4;
        x2 = background_area.get_x() + 15;
        y2 = (2 * background_area.get_y() + background_area.get_height()) / 2;
        x3 = background_area.get_x() + 7;
        y3 = background_area.get_y() + background_area.get_height() - 4;
    }
    cr->move_to(x1, y1);
    cr->line_to(x2, y2);
    cr->line_to(x3, y3);
    cr->set_source_rgb(34.0 / 255.0, 112.0 / 255.0, 1.0);
    cr->stroke();

    Gtk::Requisition text_minimum, text_natural;
    m_renderer_text.get_preferred_size(widget, text_minimum, text_natural);

    const int text_x = background_area.get_x() + 22;
    const int text_y = background_area.get_y() + background_area.get_height() / 2 - text_natural.height / 2;
    const int text_w = text_natural.width;
    const int text_h = text_natural.height;

    Gdk::Rectangle text_cell_area(text_x, text_y, text_w, text_h);

    m_renderer_text.render(cr, widget, background_area, text_cell_area, flags);
}

// text channel

void CellRendererChannels::get_preferred_width_vfunc_channel(Gtk::Widget &widget, int &minimum_width, int &natural_width) const {
    m_renderer_text.get_preferred_width(widget, minimum_width, natural_width);
}

void CellRendererChannels::get_preferred_width_for_height_vfunc_channel(Gtk::Widget &widget, int height, int &minimum_width, int &natural_width) const {
    m_renderer_text.get_preferred_width_for_height(widget, height, minimum_width, natural_width);
}

void CellRendererChannels::get_preferred_height_vfunc_channel(Gtk::Widget &widget, int &minimum_height, int &natural_height) const {
    m_renderer_text.get_preferred_height(widget, minimum_height, natural_height);
}

void CellRendererChannels::get_preferred_height_for_width_vfunc_channel(Gtk::Widget &widget, int width, int &minimum_height, int &natural_height) const {
    m_renderer_text.get_preferred_height_for_width(widget, width, minimum_height, natural_height);
}

void CellRendererChannels::render_vfunc_channel(const Cairo::RefPtr<Cairo::Context> &cr, Gtk::Widget &widget, const Gdk::Rectangle &background_area, const Gdk::Rectangle &cell_area, Gtk::CellRendererState flags) {
    Gtk::Requisition minimum_size, natural_size;
    m_renderer_text.get_preferred_size(widget, minimum_size, natural_size);

    const int text_x = background_area.get_x() + 5;
    const int text_y = background_area.get_y() + background_area.get_height() / 2 - natural_size.height / 2;
    const int text_w = natural_size.width;
    const int text_h = natural_size.height;

    Gdk::Rectangle text_cell_area(text_x, text_y, text_w, text_h);

    m_renderer_text.render(cr, widget, background_area, text_cell_area, flags);
}
