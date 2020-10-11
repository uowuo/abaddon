#include "chatmessage.hpp"
#include "../abaddon.hpp"
#include "../util.hpp"

ChatMessageItemContainer::ChatMessageItemContainer() {
    m_main = Gtk::manage(new Gtk::Box(Gtk::ORIENTATION_VERTICAL));
    add(*m_main);

    m_menu_copy_id = Gtk::manage(new Gtk::MenuItem("Copy ID"));
    m_menu_copy_id->signal_activate().connect(sigc::mem_fun(*this, &ChatMessageItemContainer::on_menu_copy_id));
    m_menu.append(*m_menu_copy_id);

    m_menu_delete_message = Gtk::manage(new Gtk::MenuItem("Delete Message"));
    m_menu_delete_message->signal_activate().connect(sigc::mem_fun(*this, &ChatMessageItemContainer::on_menu_delete_message));
    m_menu.append(*m_menu_delete_message);

    m_menu_edit_message = Gtk::manage(new Gtk::MenuItem("Edit Message"));
    m_menu_edit_message->signal_activate().connect(sigc::mem_fun(*this, &ChatMessageItemContainer::on_menu_edit_message));
    m_menu.append(*m_menu_edit_message);

    m_menu.show_all();
}

ChatMessageItemContainer *ChatMessageItemContainer::FromMessage(Snowflake id) {
    const auto *data = Abaddon::Get().GetDiscordClient().GetMessage(id);
    if (data == nullptr) return nullptr;

    auto *container = Gtk::manage(new ChatMessageItemContainer);
    container->ID = data->ID;
    container->ChannelID = data->ChannelID;

    if (data->Content.size() > 0 || data->Type != MessageType::DEFAULT) {
        container->m_text_component = container->CreateTextComponent(data);
        container->AttachMenuHandler(container->m_text_component);
        container->m_main->add(*container->m_text_component);
    }

    // there should only ever be 1 embed (i think?)
    if (data->Embeds.size() == 1) {
        container->m_embed_component = container->CreateEmbedComponent(data);
        container->AttachMenuHandler(container->m_embed_component);
        container->m_main->add(*container->m_embed_component);
    }

    // i dont think attachments can be edited
    // also this can definitely be done much better holy shit
    for (const auto &a : data->Attachments) {
        const auto last3 = a.ProxyURL.substr(a.ProxyURL.length() - 3);
        if (last3 == "png" || last3 == "jpg") {
            auto *widget = container->CreateImageComponent(a);
            auto *ev = Gtk::manage(new Gtk::EventBox);
            ev->add(*widget);
            container->AttachMenuHandler(ev);
            container->AddClickHandler(ev, a.URL);
            container->m_main->add(*ev);
            container->HandleImage(a, widget, a.ProxyURL);
        } else {
            auto *widget = container->CreateAttachmentComponent(a);
            container->AttachMenuHandler(widget);
            container->AddClickHandler(widget, a.URL);
            container->m_main->add(*widget);
        }
    }

    container->UpdateAttributes();

    return container;
}

// this doesnt rly make sense
void ChatMessageItemContainer::UpdateContent() {
    const auto *data = Abaddon::Get().GetDiscordClient().GetMessage(ID);
    if (m_text_component != nullptr)
        UpdateTextComponent(m_text_component);

    if (m_embed_component != nullptr)
        delete m_embed_component;

    if (data->Embeds.size() == 1) {
        m_embed_component = CreateEmbedComponent(data);
        if (m_embed_imgurl.size() > 0) {
            m_signal_image_load.emit(m_embed_imgurl);
        }
        AttachMenuHandler(m_embed_component);
        m_main->add(*m_embed_component);
    }
}

void ChatMessageItemContainer::UpdateImage(std::string url, Glib::RefPtr<Gdk::Pixbuf> buf) {
    if (!buf) return;

    if (m_embed_img != nullptr && m_embed_imgurl == url) {
        int w, h;
        m_embed_img->get_size_request(w, h);
        m_embed_img->property_pixbuf() = buf->scale_simple(w, h, Gdk::INTERP_BILINEAR);

        return;
    }

    auto it = m_img_loadmap.find(url);
    if (it != m_img_loadmap.end()) {
        int w, h;
        GetImageDimensions(it->second.second.Width, it->second.second.Height, w, h);
        it->second.first->property_pixbuf() = buf->scale_simple(w, h, Gdk::INTERP_BILINEAR);
    }
}

void ChatMessageItemContainer::UpdateAttributes() {
    const auto *data = Abaddon::Get().GetDiscordClient().GetMessage(ID);
    if (data == nullptr) return;

    const bool deleted = data->IsDeleted();
    const bool edited = data->IsEdited();

    if (!deleted && !edited) return;

    if (m_attrib_label == nullptr) {
        m_attrib_label = Gtk::manage(new Gtk::Label);
        m_attrib_label->set_halign(Gtk::ALIGN_START);
        m_attrib_label->show();
        m_main->add(*m_attrib_label); // todo: maybe insert markup into existing text widget's buffer if the circumstances are right (or pack horizontally)
    }

    if (deleted)
        m_attrib_label->set_markup("<span color='#ff0000'>[deleted]</span>");
    else if (edited)
        m_attrib_label->set_markup("<span color='#999999'>[edited]</span>");
}

bool ChatMessageItemContainer::EmitImageLoad(std::string url) {
    m_signal_image_load.emit(url);
    return false;
}

void ChatMessageItemContainer::AddClickHandler(Gtk::Widget *widget, std::string url) {
    // clang-format off
    widget->signal_button_press_event().connect([url](GdkEventButton *event) -> bool {
        if (event->type == Gdk::BUTTON_PRESS && event->button == GDK_BUTTON_PRIMARY) {
            LaunchBrowser(url);
            return false;
        }
        return true;
    }, false);
    // clang-format on
}

Gtk::TextView *ChatMessageItemContainer::CreateTextComponent(const Message *data) {
    auto *tv = Gtk::manage(new Gtk::TextView);

    tv->get_style_context()->add_class("message-text");
    tv->set_can_focus(false);
    tv->set_editable(false);
    tv->set_wrap_mode(Gtk::WRAP_WORD_CHAR);
    tv->set_halign(Gtk::ALIGN_FILL);
    tv->set_hexpand(true);

    UpdateTextComponent(tv);

    return tv;
}

void ChatMessageItemContainer::UpdateTextComponent(Gtk::TextView *tv) {
    const auto *data = Abaddon::Get().GetDiscordClient().GetMessage(ID);
    if (data == nullptr)
        return;

    auto b = tv->get_buffer();
    b->set_text("");
    Gtk::TextBuffer::iterator s, e;
    b->get_bounds(s, e);
    switch (data->Type) {
        case MessageType::DEFAULT:
            b->insert_markup(s, ParseMessageContent(Glib::Markup::escape_text(data->Content)));
            HandleLinks(tv);
            HandleChannelMentions(tv);
            break;
        case MessageType::GUILD_MEMBER_JOIN:
            b->insert_markup(s, "<span color='#999999'><i>[user joined]</i></span>");
            break;
        case MessageType::CHANNEL_PINNED_MESSAGE:
            b->insert_markup(s, "<span color='#999999'><i>[message pinned]</i></span>");
            break;
        default: break;
    }
}

Gtk::EventBox *ChatMessageItemContainer::CreateEmbedComponent(const Message *data) {
    Gtk::EventBox *ev = Gtk::manage(new Gtk::EventBox);
    ev->set_can_focus(true);
    Gtk::Box *main = Gtk::manage(new Gtk::Box(Gtk::ORIENTATION_VERTICAL));
    const auto &embed = data->Embeds[0];

    if (embed.Author.Name.length() > 0) {
        auto *author_lbl = Gtk::manage(new Gtk::Label);
        author_lbl->set_halign(Gtk::ALIGN_START);
        author_lbl->set_line_wrap(true);
        author_lbl->set_line_wrap_mode(Pango::WRAP_WORD_CHAR);
        author_lbl->set_hexpand(false);
        author_lbl->set_text(embed.Author.Name);
        author_lbl->get_style_context()->add_class("embed-author");
        main->pack_start(*author_lbl);
    }

    if (embed.Title.length() > 0) {
        auto *title_label = Gtk::manage(new Gtk::Label);
        title_label->set_use_markup(true);
        title_label->set_markup("<b>" + Glib::Markup::escape_text(embed.Title) + "</b>");
        title_label->set_halign(Gtk::ALIGN_CENTER);
        title_label->set_hexpand(false);
        title_label->get_style_context()->add_class("embed-title");
        title_label->set_single_line_mode(false);
        title_label->set_line_wrap(true);
        title_label->set_line_wrap_mode(Pango::WRAP_WORD_CHAR);
        title_label->set_max_width_chars(50);
        main->pack_start(*title_label);
    }

    if (embed.Description.length() > 0) {
        auto *desc_label = Gtk::manage(new Gtk::Label);
        desc_label->set_text(embed.Description);
        desc_label->set_line_wrap(true);
        desc_label->set_line_wrap_mode(Pango::WRAP_WORD_CHAR);
        desc_label->set_max_width_chars(50);
        desc_label->set_halign(Gtk::ALIGN_START);
        desc_label->set_hexpand(false);
        desc_label->get_style_context()->add_class("embed-description");
        main->pack_start(*desc_label);
    }

    // todo: handle inline fields
    if (embed.Fields.size() > 0) {
        auto *flow = Gtk::manage(new Gtk::FlowBox);
        flow->set_orientation(Gtk::ORIENTATION_HORIZONTAL);
        flow->set_min_children_per_line(3);
        flow->set_max_children_per_line(3);
        flow->set_halign(Gtk::ALIGN_START);
        flow->set_hexpand(false);
        flow->set_column_spacing(10);
        flow->set_selection_mode(Gtk::SELECTION_NONE);
        main->pack_start(*flow);

        for (const auto &field : embed.Fields) {
            auto *field_box = Gtk::manage(new Gtk::Box(Gtk::ORIENTATION_VERTICAL));
            auto *field_lbl = Gtk::manage(new Gtk::Label);
            auto *field_val = Gtk::manage(new Gtk::Label);
            field_box->set_hexpand(false);
            field_box->set_halign(Gtk::ALIGN_START);
            field_box->set_valign(Gtk::ALIGN_START);
            field_lbl->set_hexpand(false);
            field_lbl->set_halign(Gtk::ALIGN_START);
            field_lbl->set_valign(Gtk::ALIGN_START);
            field_val->set_hexpand(false);
            field_val->set_halign(Gtk::ALIGN_START);
            field_val->set_valign(Gtk::ALIGN_START);
            field_lbl->set_use_markup(true);
            field_lbl->set_markup("<b>" + Glib::Markup::escape_text(field.Name) + "</b>");
            field_lbl->set_max_width_chars(20);
            field_lbl->set_line_wrap(true);
            field_lbl->set_line_wrap_mode(Pango::WRAP_WORD_CHAR);
            field_val->set_text(field.Value);
            field_val->set_max_width_chars(20);
            field_val->set_line_wrap(true);
            field_val->set_line_wrap_mode(Pango::WRAP_WORD_CHAR);
            field_box->pack_start(*field_lbl);
            field_box->pack_start(*field_val);
            field_lbl->get_style_context()->add_class("embed-field-title");
            field_val->get_style_context()->add_class("embed-field-value");
            flow->insert(*field_box, -1);
        }
    }

    bool is_img = embed.Image.URL.size() > 0;
    bool is_thumb = embed.Thumbnail.URL.size() > 0;
    if (is_img || is_thumb) {
        auto *img = Gtk::manage(new Gtk::Image);
        img->set_halign(Gtk::ALIGN_CENTER);
        int w, h;
        if (is_img)
            GetImageDimensions(embed.Image.Width, embed.Image.Height, w, h, 200, 150);
        else
            GetImageDimensions(embed.Thumbnail.Width, embed.Thumbnail.Height, w, h, 200, 150);
        img->set_size_request(w, h);
        main->pack_start(*img);
        m_embed_img = img;
        if (is_img)
            m_embed_imgurl = embed.Image.ProxyURL;
        else
            m_embed_imgurl = embed.Thumbnail.ProxyURL;
        Glib::signal_idle().connect(sigc::bind(sigc::mem_fun(*this, &ChatMessageItemContainer::EmitImageLoad), m_embed_imgurl));
    }

    if (embed.Footer.Text.length() > 0) {
        auto *footer_lbl = Gtk::manage(new Gtk::Label);
        footer_lbl->set_halign(Gtk::ALIGN_START);
        footer_lbl->set_line_wrap(true);
        footer_lbl->set_line_wrap_mode(Pango::WRAP_WORD_CHAR);
        footer_lbl->set_hexpand(false);
        footer_lbl->set_text(embed.Footer.Text);
        footer_lbl->get_style_context()->add_class("embed-footer");
        main->pack_start(*footer_lbl);
    }

    auto style = main->get_style_context();

    if (embed.Color != -1) {
        auto provider = Gtk::CssProvider::create(); // this seems wrong
        std::string css = ".embed { border-left: 2px solid #" + IntToCSSColor(embed.Color) + "; }";
        provider->load_from_data(css);
        style->add_provider(provider, GTK_STYLE_PROVIDER_PRIORITY_APPLICATION);
    }

    style->add_class("embed");

    main->set_margin_bottom(8);
    main->set_hexpand(false);
    main->set_hexpand(false);
    main->set_halign(Gtk::ALIGN_START);
    main->set_halign(Gtk::ALIGN_START);

    ev->add(*main);
    ev->show_all();

    return ev;
}

Gtk::Image *ChatMessageItemContainer::CreateImageComponent(const AttachmentData &data) {
    int w, h;
    GetImageDimensions(data.Width, data.Height, w, h);

    auto &im = Abaddon::Get().GetImageManager();
    Gtk::Image *widget = Gtk::manage(new Gtk::Image);
    widget->set_halign(Gtk::ALIGN_START);
    widget->set_size_request(w, h);

    // clang-format off
    const auto url = data.URL;
    widget->signal_button_press_event().connect([url](GdkEventButton *event) -> bool {
        if (event->type == Gdk::BUTTON_PRESS && event->button == GDK_BUTTON_PRIMARY) {
            LaunchBrowser(url);
            return false;
        }
        return true;
    }, false);
    // clang-format on

    return widget;
}

Gtk::Box *ChatMessageItemContainer::CreateAttachmentComponent(const AttachmentData &data) {
    auto *box = Gtk::manage(new Gtk::Box(Gtk::ORIENTATION_HORIZONTAL));
    auto *ev = Gtk::manage(new Gtk::EventBox);
    auto *btn = Gtk::manage(new Gtk::Label(data.Filename + " " + HumanReadableBytes(data.Bytes))); // Gtk::LinkButton flat out doesn't work :D
    box->get_style_context()->add_class("message-attachment-box");
    ev->add(*btn);
    box->add(*ev);
    return box;
}

void ChatMessageItemContainer::HandleImage(const AttachmentData &data, Gtk::Image *img, std::string url) {
    m_img_loadmap[url] = std::make_pair(img, data);
    // ask the chatwindow to call UpdateImage because dealing with lifetimes sucks
    Glib::signal_idle().connect(sigc::bind(sigc::mem_fun(*this, &ChatMessageItemContainer::EmitImageLoad), url));
}

std::string ChatMessageItemContainer::ParseMessageContent(std::string content) {
    content = ParseMentions(content);

    return content;
}

std::string ChatMessageItemContainer::ParseMentions(std::string content) {
    constexpr static const auto mentions_regex = R"(&lt;@!?(\d+)&gt;)";

    return RegexReplaceMany(content, mentions_regex, [this](const std::string &idstr) -> std::string {
        const Snowflake id(idstr);
        const auto &discord = Abaddon::Get().GetDiscordClient();
        const auto *user = discord.GetUser(id);
        const auto *channel = discord.GetChannel(ChannelID);
        if (channel == nullptr || user == nullptr) return idstr;

        if (channel->Type == ChannelType::DM || channel->Type == ChannelType::GROUP_DM)
            return "<b>@" + Glib::Markup::escape_text(user->Username) + "#" + user->Discriminator + "</b>";

        const auto colorid = user->GetHoistedRole(channel->GuildID, true);
        const auto *role = discord.GetRole(colorid);
        if (role == nullptr)
            return "<b>@" + Glib::Markup::escape_text(user->Username) + "#" + user->Discriminator + "</b>";

        return "<b><span color=\"#" + IntToCSSColor(role->Color) + "\">@" + Glib::Markup::escape_text(user->Username) + "#" + user->Discriminator + "</span></b>";
    });
}

void ChatMessageItemContainer::HandleChannelMentions(Gtk::TextView *tv) {
    constexpr static const auto chan_regex = R"(<#(\d+)>)";

    std::regex rgx(chan_regex, std::regex_constants::ECMAScript);

    tv->signal_button_press_event().connect(sigc::mem_fun(*this, &ChatMessageItemContainer::OnClickChannel), false);

    auto buf = tv->get_buffer();
    std::string text = buf->get_text();

    const auto &discord = Abaddon::Get().GetDiscordClient();

    std::string::const_iterator sstart(text.begin());
    std::smatch match;
    while (std::regex_search(sstart, text.cend(), match, rgx)) {
        std::string channel_id = match.str(1);
        const auto *chan = discord.GetChannel(channel_id);
        if (chan == nullptr) {
            sstart = match.suffix().first;
            continue;
        }

        auto tag = buf->create_tag();
        m_channel_tagmap[tag] = channel_id;
        tag->property_weight() = Pango::WEIGHT_BOLD;

        const auto start = std::distance(text.cbegin(), sstart) + match.position();
        auto erase_from = buf->get_iter_at_offset(start);
        auto erase_to = buf->get_iter_at_offset(start + match.length());
        auto it = buf->erase(erase_from, erase_to);
        const std::string replacement = "#" + chan->Name;
        it = buf->insert_with_tag(it, "#" + chan->Name, tag);

        // rescan the whole thing so i dont have to deal with fixing match positions
        text = buf->get_text();
        sstart = text.begin();
    }
}

// a lot of repetition here so there should probably just be one slot for textview's button-press
bool ChatMessageItemContainer::OnClickChannel(GdkEventButton *ev) {
    if (m_text_component == nullptr) return false;
    if (ev->type != Gdk::BUTTON_PRESS) return false;
    if (ev->button != GDK_BUTTON_PRIMARY) return false;

    auto buf = m_text_component->get_buffer();
    Gtk::TextBuffer::iterator start, end;
    buf->get_selection_bounds(start, end); // no open if selection
    if (start.get_offset() != end.get_offset())
        return false;

    int x, y;
    m_text_component->window_to_buffer_coords(Gtk::TEXT_WINDOW_WIDGET, ev->x, ev->y, x, y);
    Gtk::TextBuffer::iterator iter;
    m_text_component->get_iter_at_location(iter, x, y);

    const auto tags = iter.get_tags();
    for (auto tag : tags) {
        const auto it = m_channel_tagmap.find(tag);
        if (it != m_channel_tagmap.end()) {
            m_signal_action_channel_click.emit(it->second);

            return true;
        }
    }

    return false;
}

void ChatMessageItemContainer::HandleLinks(Gtk::TextView *tv) {
    constexpr static const auto links_regex = R"(\bhttps?:\/\/[^\s]+\.[^\s]+\b)";

    std::regex rgx(links_regex, std::regex_constants::ECMAScript);

    tv->signal_button_press_event().connect(sigc::mem_fun(*this, &ChatMessageItemContainer::OnLinkClick), false);

    auto buf = tv->get_buffer();
    Gtk::TextBuffer::iterator start, end;
    buf->get_bounds(start, end);
    std::string text = buf->get_slice(start, end);

    // i'd like to let this be done thru css like .message-link { color: #bitch; } but idk how
    auto &settings = Abaddon::Get().GetSettings();
    auto link_color = settings.GetSettingString("misc", "linkcolor", "rgba(40, 200, 180, 255)");

    std::string::const_iterator sstart(text.begin());
    std::smatch match;
    while (std::regex_search(sstart, text.cend(), match, rgx)) {
        std::string link = match.str();
        auto tag = buf->create_tag();
        m_link_tagmap[tag] = link;
        tag->property_foreground_rgba() = Gdk::RGBA(link_color);

        const auto start = std::distance(text.cbegin(), sstart) + match.position();
        auto erase_from = buf->get_iter_at_offset(start);
        auto erase_to = buf->get_iter_at_offset(start + match.length());
        auto it = buf->erase(erase_from, erase_to);
        it = buf->insert_with_tag(it, link, tag);

        sstart = match.suffix().first;
    }
}

bool ChatMessageItemContainer::OnLinkClick(GdkEventButton *ev) {
    if (m_text_component == nullptr) return false;
    if (ev->type != Gdk::BUTTON_PRESS) return false;
    if (ev->button != GDK_BUTTON_PRIMARY) return false;

    auto buf = m_text_component->get_buffer();
    Gtk::TextBuffer::iterator start, end;
    buf->get_selection_bounds(start, end); // no open if selection
    if (start.get_offset() != end.get_offset())
        return false;

    int x, y;
    m_text_component->window_to_buffer_coords(Gtk::TEXT_WINDOW_WIDGET, ev->x, ev->y, x, y);
    Gtk::TextBuffer::iterator iter;
    m_text_component->get_iter_at_location(iter, x, y);

    const auto tags = iter.get_tags();
    for (auto tag : tags) {
        const auto it = m_link_tagmap.find(tag);
        if (it != m_link_tagmap.end()) {
            LaunchBrowser(it->second);

            return true;
        }
    }

    return false;
}

void ChatMessageItemContainer::ShowMenu(GdkEvent *event) {
    const auto &client = Abaddon::Get().GetDiscordClient();
    const auto *data = client.GetMessage(ID);
    if (data->IsDeleted()) {
        m_menu_delete_message->set_sensitive(false);
        m_menu_edit_message->set_sensitive(false);
    } else {
        const bool can_edit = client.GetUserData().ID == data->Author.ID;
        const bool can_delete = can_edit || client.HasChannelPermission(client.GetUserData().ID, ChannelID, Permission::MANAGE_MESSAGES);
        m_menu_delete_message->set_sensitive(can_delete);
        m_menu_edit_message->set_sensitive(can_edit);
    }

    m_menu.popup_at_pointer(event);
}

void ChatMessageItemContainer::on_menu_copy_id() {
    Gtk::Clipboard::get()->set_text(std::to_string(ID));
}

void ChatMessageItemContainer::on_menu_delete_message() {
    m_signal_action_delete.emit();
}

void ChatMessageItemContainer::on_menu_edit_message() {
    m_signal_action_edit.emit();
}

ChatMessageItemContainer::type_signal_action_delete ChatMessageItemContainer::signal_action_delete() {
    return m_signal_action_delete;
}

ChatMessageItemContainer::type_signal_action_edit ChatMessageItemContainer::signal_action_edit() {
    return m_signal_action_edit;
}

ChatMessageItemContainer::type_signal_channel_click ChatMessageItemContainer::signal_action_channel_click() {
    return m_signal_action_channel_click;
}

ChatMessageItemContainer::type_signal_image_load ChatMessageItemContainer::signal_image_load() {
    return m_signal_image_load;
}

// clang-format off
void ChatMessageItemContainer::AttachMenuHandler(Gtk::Widget *widget) {
    widget->signal_button_press_event().connect([this](GdkEventButton *event) -> bool {
        if (event->type == GDK_BUTTON_PRESS && event->button == GDK_BUTTON_SECONDARY) {
            ShowMenu(reinterpret_cast<GdkEvent*>(event));
            return true;
        }

        return false;
    }, false);
}
// clang-format on

ChatMessageHeader::ChatMessageHeader(const Message *data) {
    UserID = data->Author.ID;
    ChannelID = data->ChannelID;

    m_main_box = Gtk::manage(new Gtk::Box(Gtk::ORIENTATION_HORIZONTAL));
    m_content_box = Gtk::manage(new Gtk::Box(Gtk::ORIENTATION_VERTICAL));
    m_meta_box = Gtk::manage(new Gtk::Box(Gtk::ORIENTATION_HORIZONTAL));
    m_author = Gtk::manage(new Gtk::Label);
    m_timestamp = Gtk::manage(new Gtk::Label);

    auto buf = Abaddon::Get().GetImageManager().GetFromURLIfCached(data->Author.GetAvatarURL());
    if (buf)
        m_avatar = Gtk::manage(new Gtk::Image(buf));
    else
        m_avatar = Gtk::manage(new Gtk::Image(Abaddon::Get().GetImageManager().GetPlaceholder(32)));

    get_style_context()->add_class("message-container");
    m_author->get_style_context()->add_class("message-container-author");
    m_timestamp->get_style_context()->add_class("message-container-timestamp");
    m_avatar->get_style_context()->add_class("message-container-avatar");

    m_avatar->set_valign(Gtk::ALIGN_START);
    m_avatar->set_margin_right(10);

    m_author->set_markup("<span weight='bold'>" + Glib::Markup::escape_text(data->Author.Username) + "</span>");
    m_author->set_single_line_mode(true);
    m_author->set_line_wrap(false);
    m_author->set_ellipsize(Pango::ELLIPSIZE_END);
    m_author->set_xalign(0.f);
    m_author->set_can_focus(false);

    if (data->WebhookID.IsValid()) {
        m_extra = Gtk::manage(new Gtk::Label);
        m_extra->get_style_context()->add_class("message-container-extra");
        m_extra->set_single_line_mode(true);
        m_extra->set_margin_start(12);
        m_extra->set_can_focus(false);
        m_extra->set_use_markup(true);
        m_extra->set_markup("<b>Webhook</b>");
    } else if (data->Author.IsBot) {
        m_extra = Gtk::manage(new Gtk::Label);
        m_extra->get_style_context()->add_class("message-container-extra");
        m_extra->set_single_line_mode(true);
        m_extra->set_margin_start(12);
        m_extra->set_can_focus(false);
        m_extra->set_use_markup(true);
        m_extra->set_markup("<b>BOT</b>");
    }

    m_timestamp->set_text(data->Timestamp);
    m_timestamp->set_opacity(0.5);
    m_timestamp->set_single_line_mode(true);
    m_timestamp->set_margin_start(12);
    m_timestamp->set_can_focus(false);

    m_main_box->set_hexpand(true);
    m_main_box->set_vexpand(true);
    m_main_box->set_can_focus(true);

    m_meta_box->set_can_focus(false);

    m_content_box->set_can_focus(false);

    m_meta_box->add(*m_author);
    if (m_extra != nullptr)
        m_meta_box->add(*m_extra);
    m_meta_box->add(*m_timestamp);
    m_content_box->add(*m_meta_box);
    m_main_box->add(*m_avatar);
    m_main_box->add(*m_content_box);
    add(*m_main_box);

    set_margin_bottom(8);

    show_all();

    UpdateNameColor();
}

void ChatMessageHeader::UpdateNameColor() {
    const auto &discord = Abaddon::Get().GetDiscordClient();
    const auto guild_id = discord.GetChannel(ChannelID)->GuildID;
    const auto role_id = discord.GetMemberHoistedRole(guild_id, UserID, true);
    const auto *user = discord.GetUser(UserID);
    if (user == nullptr) return;
    const auto *role = discord.GetRole(role_id);

    std::string md;
    if (role != nullptr)
        md = "<span weight='bold' color='#" + IntToCSSColor(role->Color) + "'>" + Glib::Markup::escape_text(user->Username) + "</span>";
    else
        md = "<span weight='bold' color='#eeeeee'>" + Glib::Markup::escape_text(user->Username) + "</span>";

    m_author->set_markup(md);
}

void ChatMessageHeader::AddContent(Gtk::Widget *widget, bool prepend) {
    m_content_box->add(*widget);
    if (prepend)
        m_content_box->reorder_child(*widget, 1);
}

void ChatMessageHeader::SetAvatarFromPixbuf(Glib::RefPtr<Gdk::Pixbuf> pixbuf) {
    m_avatar->property_pixbuf() = pixbuf;
}
