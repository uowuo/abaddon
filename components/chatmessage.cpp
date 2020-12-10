#include "chatmessage.hpp"
#include "../abaddon.hpp"
#include "../util.hpp"
#include <unordered_map>

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

    m_menu_copy_content = Gtk::manage(new Gtk::MenuItem("Copy Content"));
    m_menu_copy_content->signal_activate().connect(sigc::mem_fun(*this, &ChatMessageItemContainer::on_menu_copy_content));
    m_menu.append(*m_menu_copy_content);

    m_menu.show_all();

    m_link_menu_copy = Gtk::manage(new Gtk::MenuItem("Copy Link"));
    m_link_menu_copy->signal_activate().connect(sigc::mem_fun(*this, &ChatMessageItemContainer::on_link_menu_copy));
    m_link_menu.append(*m_link_menu_copy);

    m_link_menu.show_all();
}

ChatMessageItemContainer *ChatMessageItemContainer::FromMessage(Snowflake id) {
    const auto data = Abaddon::Get().GetDiscordClient().GetMessage(id);
    if (!data.has_value()) return nullptr;

    auto *container = Gtk::manage(new ChatMessageItemContainer);
    container->ID = data->ID;
    container->ChannelID = data->ChannelID;

    if (data->Content.size() > 0 || data->Type != MessageType::DEFAULT) {
        container->m_text_component = container->CreateTextComponent(&*data);
        container->AttachGuildMenuHandler(container->m_text_component);
        container->m_main->add(*container->m_text_component);
    }

    // there should only ever be 1 embed (i think?)
    if (data->Embeds.size() == 1) {
        container->m_embed_component = container->CreateEmbedComponent(&*data);
        container->AttachGuildMenuHandler(container->m_embed_component);
        container->m_main->add(*container->m_embed_component);
    }

    // i dont think attachments can be edited
    // also this can definitely be done much better holy shit
    for (const auto &a : data->Attachments) {
        const auto last3 = a.ProxyURL.substr(a.ProxyURL.length() - 3);
        if (last3 == "png" || last3 == "jpg") {
            auto *widget = container->CreateImageComponent(a);
            container->m_main->add(*widget);
        } else {
            auto *widget = container->CreateAttachmentComponent(a);
            container->m_main->add(*widget);
        }
    }

    // only 1?
    if (data->Stickers.has_value()) {
        const auto &sticker = data->Stickers.value()[0];
        // todo: lottie, proper apng
        if (sticker.FormatType == StickerFormatType::PNG || sticker.FormatType == StickerFormatType::APNG) {
            auto *widget = container->CreateStickerComponent(sticker);
            container->m_main->add(*widget);
        }
    }

    container->UpdateAttributes();

    return container;
}

// this doesnt rly make sense
void ChatMessageItemContainer::UpdateContent() {
    const auto data = Abaddon::Get().GetDiscordClient().GetMessage(ID);
    if (m_text_component != nullptr)
        UpdateTextComponent(m_text_component);

    if (m_embed_component != nullptr) {
        delete m_embed_component;
        m_embed_component = nullptr;
    }

    if (data->Embeds.size() == 1) {
        m_embed_component = CreateEmbedComponent(&*data);
        if (m_embed_imgurl.size() > 0) {
            m_signal_image_load.emit(m_embed_imgurl);
        }
        AttachGuildMenuHandler(m_embed_component);
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
        const auto inw = it->second.second.Width;
        const auto inh = it->second.second.Height;
        if (inw.has_value() && inh.has_value()) {
            int w, h;
            GetImageDimensions(*inw, *inh, w, h);
            it->second.first->property_pixbuf() = buf->scale_simple(w, h, Gdk::INTERP_BILINEAR);
        }
    }
}

void ChatMessageItemContainer::UpdateAttributes() {
    const auto data = Abaddon::Get().GetDiscordClient().GetMessage(ID);
    if (!data.has_value()) return;

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
    const auto data = Abaddon::Get().GetDiscordClient().GetMessage(ID);
    if (!data.has_value()) return;

    auto b = tv->get_buffer();
    b->set_text("");
    Gtk::TextBuffer::iterator s, e;
    b->get_bounds(s, e);
    switch (data->Type) {
        case MessageType::DEFAULT:
            b->insert(s, data->Content);
            HandleUserMentions(tv);
            HandleLinks(tv);
            HandleChannelMentions(tv);
            HandleEmojis(tv);
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

Gtk::Widget *ChatMessageItemContainer::CreateEmbedComponent(const Message *data) {
    Gtk::EventBox *ev = Gtk::manage(new Gtk::EventBox);
    ev->set_can_focus(true);
    Gtk::Box *main = Gtk::manage(new Gtk::Box(Gtk::ORIENTATION_VERTICAL));
    const auto &embed = data->Embeds[0];

    if (embed.Author.has_value() && embed.Author->Name.has_value()) {
        auto *author_lbl = Gtk::manage(new Gtk::Label);
        author_lbl->set_halign(Gtk::ALIGN_START);
        author_lbl->set_line_wrap(true);
        author_lbl->set_line_wrap_mode(Pango::WRAP_WORD_CHAR);
        author_lbl->set_hexpand(false);
        author_lbl->set_text(*embed.Author->Name);
        author_lbl->get_style_context()->add_class("embed-author");
        main->pack_start(*author_lbl);
    }

    if (embed.Title.has_value()) {
        auto *title_label = Gtk::manage(new Gtk::Label);
        title_label->set_use_markup(true);
        title_label->set_markup("<b>" + Glib::Markup::escape_text(*embed.Title) + "</b>");
        title_label->set_halign(Gtk::ALIGN_CENTER);
        title_label->set_hexpand(false);
        title_label->get_style_context()->add_class("embed-title");
        title_label->set_single_line_mode(false);
        title_label->set_line_wrap(true);
        title_label->set_line_wrap_mode(Pango::WRAP_WORD_CHAR);
        title_label->set_max_width_chars(50);
        main->pack_start(*title_label);
    }

    if (embed.Description.has_value()) {
        auto *desc_label = Gtk::manage(new Gtk::Label);
        desc_label->set_text(*embed.Description);
        desc_label->set_line_wrap(true);
        desc_label->set_line_wrap_mode(Pango::WRAP_WORD_CHAR);
        desc_label->set_max_width_chars(50);
        desc_label->set_halign(Gtk::ALIGN_START);
        desc_label->set_hexpand(false);
        desc_label->get_style_context()->add_class("embed-description");
        main->pack_start(*desc_label);
    }

    // todo: handle inline fields
    if (embed.Fields.has_value() && embed.Fields->size() > 0) {
        auto *flow = Gtk::manage(new Gtk::FlowBox);
        flow->set_orientation(Gtk::ORIENTATION_HORIZONTAL);
        flow->set_min_children_per_line(3);
        flow->set_max_children_per_line(3);
        flow->set_halign(Gtk::ALIGN_START);
        flow->set_hexpand(false);
        flow->set_column_spacing(10);
        flow->set_selection_mode(Gtk::SELECTION_NONE);
        main->pack_start(*flow);

        for (const auto &field : *embed.Fields) {
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

    if (embed.Image.has_value()) {
        bool is_img = embed.Image->URL.has_value();
        bool is_thumb = embed.Thumbnail.has_value();
        if (is_img || is_thumb) {
            auto *img = Gtk::manage(new Gtk::Image);
            img->set_halign(Gtk::ALIGN_CENTER);
            int w = 0, h = 0;
            if (is_img)
                GetImageDimensions(*embed.Image->Width, *embed.Image->Height, w, h, 200, 150);
            else
                GetImageDimensions(*embed.Thumbnail->Width, *embed.Thumbnail->Height, w, h, 200, 150);
            img->set_size_request(w, h);
            main->pack_start(*img);
            m_embed_img = img;
            if (is_img)
                m_embed_imgurl = *embed.Image->ProxyURL;
            else
                m_embed_imgurl = *embed.Thumbnail->ProxyURL;
            Glib::signal_idle().connect(sigc::bind(sigc::mem_fun(*this, &ChatMessageItemContainer::EmitImageLoad), m_embed_imgurl));
        }
    }

    if (embed.Footer.has_value()) {
        auto *footer_lbl = Gtk::manage(new Gtk::Label);
        footer_lbl->set_halign(Gtk::ALIGN_START);
        footer_lbl->set_line_wrap(true);
        footer_lbl->set_line_wrap_mode(Pango::WRAP_WORD_CHAR);
        footer_lbl->set_hexpand(false);
        footer_lbl->set_text(embed.Footer->Text);
        footer_lbl->get_style_context()->add_class("embed-footer");
        main->pack_start(*footer_lbl);
    }

    auto style = main->get_style_context();

    if (embed.Color.has_value()) {
        auto provider = Gtk::CssProvider::create(); // this seems wrong
        std::string css = ".embed { border-left: 2px solid #" + IntToCSSColor(*embed.Color) + "; }";
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

Gtk::Widget *ChatMessageItemContainer::CreateImageComponent(const AttachmentData &data) {
    int w, h;
    GetImageDimensions(*data.Width, *data.Height, w, h);

    Gtk::EventBox *ev = Gtk::manage(new Gtk::EventBox);
    Gtk::Image *widget = Gtk::manage(new Gtk::Image);
    ev->add(*widget);
    widget->set_halign(Gtk::ALIGN_START);
    widget->set_size_request(w, h);

    AttachGuildMenuHandler(ev);
    AddClickHandler(ev, data.URL);
    HandleImage(data, widget, data.ProxyURL);

    return ev;
}

Gtk::Widget *ChatMessageItemContainer::CreateAttachmentComponent(const AttachmentData &data) {
    auto *ev = Gtk::manage(new Gtk::EventBox);
    auto *btn = Gtk::manage(new Gtk::Label(data.Filename + " " + HumanReadableBytes(data.Bytes))); // Gtk::LinkButton flat out doesn't work :D
    ev->get_style_context()->add_class("message-attachment-box");
    ev->add(*btn);

    AttachGuildMenuHandler(ev);
    AddClickHandler(ev, data.URL);

    return ev;
}

Gtk::Widget *ChatMessageItemContainer::CreateStickerComponent(const Sticker &data) {
    auto *box = Gtk::manage(new Gtk::Box(Gtk::ORIENTATION_HORIZONTAL));
    auto *imgw = Gtk::manage(new Gtk::Image);
    box->add(*imgw);
    auto &img = Abaddon::Get().GetImageManager();

    if (data.FormatType == StickerFormatType::PNG || data.FormatType == StickerFormatType::APNG) {
        // clang-format off
        img.LoadFromURL(data.GetURL(), sigc::track_obj([this, imgw](const Glib::RefPtr<Gdk::Pixbuf> &pixbuf) {
            imgw->property_pixbuf() = pixbuf;
        }, imgw));
        // clang-format on
    }

    AttachGuildMenuHandler(box);
    return box;
}

void ChatMessageItemContainer::HandleImage(const AttachmentData &data, Gtk::Image *img, std::string url) {
    m_img_loadmap[url] = std::make_pair(img, data);
    // ask the chatwindow to call UpdateImage because dealing with lifetimes sucks
    Glib::signal_idle().connect(sigc::bind(sigc::mem_fun(*this, &ChatMessageItemContainer::EmitImageLoad), url));
}

Glib::ustring ChatMessageItemContainer::GetText(const Glib::RefPtr<Gtk::TextBuffer> &buf) {
    Gtk::TextBuffer::iterator a, b;
    buf->get_bounds(a, b);
    auto slice = buf->get_slice(a, b, true);
    return slice;
}

void ChatMessageItemContainer::HandleUserMentions(Gtk::TextView *tv) {
    constexpr static const auto mentions_regex = R"(<@!?(\d+)>)";

    static auto rgx = Glib::Regex::create(mentions_regex);

    auto buf = tv->get_buffer();
    Glib::ustring text = GetText(buf);
    const auto &discord = Abaddon::Get().GetDiscordClient();

    int startpos = 0;
    Glib::MatchInfo match;
    while (rgx->match(text, startpos, match)) {
        int mstart, mend;
        if (!match.fetch_pos(0, mstart, mend)) break;
        const Glib::ustring user_id = match.fetch(1);
        const auto user = discord.GetUser(user_id);
        const auto channel = discord.GetChannel(ChannelID);
        if (!user.has_value() || !channel.has_value()) {
            startpos = mend;
            continue;
        }

        Glib::ustring replacement;

        if (channel->Type == ChannelType::DM || channel->Type == ChannelType::GROUP_DM)
            replacement = "<b>@" + Glib::Markup::escape_text(user->Username) + "#" + user->Discriminator + "</b>";
        else {
            const auto role_id = user->GetHoistedRole(*channel->GuildID, true);
            const auto role = discord.GetRole(role_id);
            if (!role.has_value())
                replacement = "<b>@" + Glib::Markup::escape_text(user->Username) + "#" + user->Discriminator + "</b>";
            else
                replacement = "<b><span color=\"#" + IntToCSSColor(role->Color) + "\">@" + Glib::Markup::escape_text(user->Username) + "#" + user->Discriminator + "</span></b>";
        }

        // regex returns byte positions and theres no straightforward way in the c++ bindings to deal with that :(
        const auto chars_start = g_utf8_pointer_to_offset(text.c_str(), text.c_str() + mstart);
        const auto chars_end = g_utf8_pointer_to_offset(text.c_str(), text.c_str() + mend);
        const auto start_it = buf->get_iter_at_offset(chars_start);
        const auto end_it = buf->get_iter_at_offset(chars_end);

        auto it = buf->erase(start_it, end_it);
        buf->insert_markup(it, replacement);

        text = GetText(buf);
        startpos = 0;
    }
}

void ChatMessageItemContainer::HandleStockEmojis(Gtk::TextView *tv) {
    Abaddon::Get().GetEmojis().ReplaceEmojis(tv->get_buffer());
}

void ChatMessageItemContainer::HandleCustomEmojis(Gtk::TextView *tv) {
    static auto rgx = Glib::Regex::create(R"(<a?:([\w\d_]+):(\d+)>)");

    auto &img = Abaddon::Get().GetImageManager();

    auto buf = tv->get_buffer();
    auto text = GetText(buf);

    Glib::MatchInfo match;
    int startpos = 0;
    while (rgx->match(text, startpos, match)) {
        int mstart, mend;
        if (!match.fetch_pos(0, mstart, mend)) break;

        const auto chars_start = g_utf8_pointer_to_offset(text.c_str(), text.c_str() + mstart);
        const auto chars_end = g_utf8_pointer_to_offset(text.c_str(), text.c_str() + mend);
        auto start_it = buf->get_iter_at_offset(chars_start);
        auto end_it = buf->get_iter_at_offset(chars_end);

        startpos = mend;
        auto pixbuf = img.GetFromURLIfCached(Emoji::URLFromID(match.fetch(2)));
        if (pixbuf) {
            auto it = buf->erase(start_it, end_it);
            int alen = text.size();
            text = GetText(buf);
            int blen = text.size();
            startpos -= (alen - blen);
            buf->insert_pixbuf(it, pixbuf->scale_simple(24, 24, Gdk::INTERP_BILINEAR));
        } else {
            // clang-format off
            // can't erase before pixbuf is ready or else marks that are in the same pos get mixed up
            auto mark_start = buf->create_mark(start_it, false);
            end_it.backward_char();
            auto mark_end = buf->create_mark(end_it, false);
            img.LoadFromURL(Emoji::URLFromID(match.fetch(2)), sigc::track_obj([this, buf, mark_start, mark_end](Glib::RefPtr<Gdk::Pixbuf> pixbuf) {
                auto start_it = mark_start->get_iter();
                auto end_it = mark_end->get_iter();
                end_it.forward_char();
                buf->delete_mark(mark_start);
                buf->delete_mark(mark_end);
                auto it = buf->erase(start_it, end_it);
                buf->insert_pixbuf(it, pixbuf->scale_simple(24, 24, Gdk::INTERP_BILINEAR));
            }, tv));
            // clang-format on
        }

        text = GetText(buf);
    }
}

void ChatMessageItemContainer::HandleEmojis(Gtk::TextView *tv) {
    static bool emojis = Abaddon::Get().GetSettings().GetSettingBool("gui", "emojis", true);
    if (emojis) {
        HandleStockEmojis(tv);
        HandleCustomEmojis(tv);
    }
}

void ChatMessageItemContainer::HandleChannelMentions(Gtk::TextView *tv) {
    static auto rgx = Glib::Regex::create(R"(<#(\d+)>)");

    tv->signal_button_press_event().connect(sigc::mem_fun(*this, &ChatMessageItemContainer::OnClickChannel), false);

    auto buf = tv->get_buffer();
    Glib::ustring text = GetText(buf);

    const auto &discord = Abaddon::Get().GetDiscordClient();

    int startpos = 0;
    Glib::MatchInfo match;
    while (rgx->match(text, startpos, match)) {
        int mstart, mend;
        match.fetch_pos(0, mstart, mend);
        std::string channel_id = match.fetch(1);
        const auto chan = discord.GetChannel(channel_id);
        if (!chan.has_value()) {
            startpos = mend;
            continue;
        }

        auto tag = buf->create_tag();
        m_channel_tagmap[tag] = channel_id;
        tag->property_weight() = Pango::WEIGHT_BOLD;

        const auto chars_start = g_utf8_pointer_to_offset(text.c_str(), text.c_str() + mstart);
        const auto chars_end = g_utf8_pointer_to_offset(text.c_str(), text.c_str() + mend);
        const auto erase_from = buf->get_iter_at_offset(chars_start);
        const auto erase_to = buf->get_iter_at_offset(chars_end);
        auto it = buf->erase(erase_from, erase_to);
        const std::string replacement = "#" + *chan->Name;
        it = buf->insert_with_tag(it, "#" + *chan->Name, tag);

        // rescan the whole thing so i dont have to deal with fixing match positions
        text = GetText(buf);
        startpos = 0;
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

void ChatMessageItemContainer::on_link_menu_copy() {
    Gtk::Clipboard::get()->set_text(m_selected_link);
}

void ChatMessageItemContainer::HandleLinks(Gtk::TextView *tv) {
    const auto rgx = Glib::Regex::create(R"(\bhttps?:\/\/[^\s]+\.[^\s]+\b)");

    tv->signal_button_press_event().connect(sigc::mem_fun(*this, &ChatMessageItemContainer::OnLinkClick), false);

    auto buf = tv->get_buffer();
    Glib::ustring text = GetText(buf);

    // i'd like to let this be done thru css like .message-link { color: #bitch; } but idk how
    auto &settings = Abaddon::Get().GetSettings();
    static auto link_color = settings.GetSettingString("misc", "linkcolor", "rgba(40, 200, 180, 255)");

    int startpos = 0;
    Glib::MatchInfo match;
    while (rgx->match(text, startpos, match)) {
        int mstart, mend;
        match.fetch_pos(0, mstart, mend);
        std::string link = match.fetch(0);
        auto tag = buf->create_tag();
        m_link_tagmap[tag] = link;
        tag->property_foreground_rgba() = Gdk::RGBA(link_color);
        tag->set_property("underline", 1); // stupid workaround for vcpkg bug (i think)

        const auto chars_start = g_utf8_pointer_to_offset(text.c_str(), text.c_str() + mstart);
        const auto chars_end = g_utf8_pointer_to_offset(text.c_str(), text.c_str() + mend);
        const auto erase_from = buf->get_iter_at_offset(chars_start);
        const auto erase_to = buf->get_iter_at_offset(chars_end);
        auto it = buf->erase(erase_from, erase_to);
        it = buf->insert_with_tag(it, link, tag);

        startpos = mend;
    }
}

bool ChatMessageItemContainer::OnLinkClick(GdkEventButton *ev) {
    if (m_text_component == nullptr) return false;
    if (ev->type != Gdk::BUTTON_PRESS) return false;
    if (ev->button != GDK_BUTTON_PRIMARY && ev->button != GDK_BUTTON_SECONDARY) return false;

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
            if (ev->button == GDK_BUTTON_PRIMARY) {
                LaunchBrowser(it->second);
                return true;
            } else if (ev->button == GDK_BUTTON_SECONDARY) {
                m_selected_link = it->second;
                m_link_menu.popup_at_pointer(reinterpret_cast<GdkEvent *>(ev));
                return true;
            }
        }
    }

    return false;

    return false;
}

void ChatMessageItemContainer::ShowMenu(GdkEvent *event) {
    const auto &client = Abaddon::Get().GetDiscordClient();
    const auto data = client.GetMessage(ID);
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

void ChatMessageItemContainer::on_menu_copy_content() {
    const auto msg = Abaddon::Get().GetDiscordClient().GetMessage(ID);
    if (msg.has_value())
        Gtk::Clipboard::get()->set_text(msg->Content);
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

void ChatMessageItemContainer::AttachGuildMenuHandler(Gtk::Widget *widget) {
    // clang-format off
    widget->signal_button_press_event().connect([this](GdkEventButton *event) -> bool {
        if (event->type == GDK_BUTTON_PRESS && event->button == GDK_BUTTON_SECONDARY) {
            ShowMenu(reinterpret_cast<GdkEvent*>(event));
            return true;
        }

        return false;
    }, false);
    // clang-format on
}

ChatMessageHeader::ChatMessageHeader(const Message *data) {
    UserID = data->Author.ID;
    ChannelID = data->ChannelID;

    m_main_box = Gtk::manage(new Gtk::Box(Gtk::ORIENTATION_HORIZONTAL));
    m_content_box = Gtk::manage(new Gtk::Box(Gtk::ORIENTATION_VERTICAL));
    m_meta_box = Gtk::manage(new Gtk::Box(Gtk::ORIENTATION_HORIZONTAL));
    m_meta_ev = Gtk::manage(new Gtk::EventBox);
    m_author = Gtk::manage(new Gtk::Label);
    m_timestamp = Gtk::manage(new Gtk::Label);
    m_avatar_ev = Gtk::manage(new Gtk::EventBox);

    const auto author = Abaddon::Get().GetDiscordClient().GetUser(UserID);
    auto &img = Abaddon::Get().GetImageManager();
    Glib::RefPtr<Gdk::Pixbuf> buf;
    if (author.has_value())
        buf = img.GetFromURLIfCached(author->GetAvatarURL());

    if (buf)
        m_avatar = Gtk::manage(new Gtk::Image(buf));
    else {
        m_avatar = Gtk::manage(new Gtk::Image(img.GetPlaceholder(32)));
        if (author.has_value())
            img.LoadFromURL(author->GetAvatarURL(), sigc::mem_fun(*this, &ChatMessageHeader::OnAvatarLoad));
    }

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

    m_meta_ev->signal_button_press_event().connect(sigc::mem_fun(*this, &ChatMessageHeader::on_author_button_press));

    if (data->WebhookID.has_value()) {
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
    m_meta_ev->add(*m_meta_box);
    m_content_box->add(*m_meta_ev);
    m_avatar_ev->add(*m_avatar);
    m_main_box->add(*m_avatar_ev);
    m_main_box->add(*m_content_box);
    add(*m_main_box);

    set_margin_bottom(8);

    show_all();

    UpdateNameColor();
    AttachUserMenuHandler(*m_meta_ev);
    AttachUserMenuHandler(*m_avatar_ev);
}

void ChatMessageHeader::UpdateNameColor() {
    const auto &discord = Abaddon::Get().GetDiscordClient();
    const auto guild_id = discord.GetChannel(ChannelID)->GuildID;
    const auto role_id = discord.GetMemberHoistedRole(*guild_id, UserID, true);
    const auto user = discord.GetUser(UserID);
    if (!user.has_value()) return;
    const auto role = discord.GetRole(role_id);

    std::string md;
    if (role.has_value())
        md = "<span weight='bold' color='#" + IntToCSSColor(role->Color) + "'>" + Glib::Markup::escape_text(user->Username) + "</span>";
    else
        md = "<span weight='bold' color='#eeeeee'>" + Glib::Markup::escape_text(user->Username) + "</span>";

    m_author->set_markup(md);
}

void ChatMessageHeader::OnAvatarLoad(const Glib::RefPtr<Gdk::Pixbuf> &pixbuf) {
    m_avatar->property_pixbuf() = pixbuf;
}

void ChatMessageHeader::AttachUserMenuHandler(Gtk::Widget &widget) {
    widget.signal_button_press_event().connect([this](GdkEventButton *ev) -> bool {
        if (ev->type == GDK_BUTTON_PRESS && ev->button == GDK_BUTTON_SECONDARY) {
            m_signal_action_open_user_menu.emit(reinterpret_cast<GdkEvent *>(ev));
            return true;
        }

        return false;
    });
}

bool ChatMessageHeader::on_author_button_press(GdkEventButton *ev) {
    if (ev->button == GDK_BUTTON_PRIMARY && (ev->state & GDK_SHIFT_MASK)) {
        m_signal_action_insert_mention.emit();
        return true;
    }

    return false;
}

ChatMessageHeader::type_signal_action_insert_mention ChatMessageHeader::signal_action_insert_mention() {
    return m_signal_action_insert_mention;
}

ChatMessageHeader::type_signal_action_open_user_menu ChatMessageHeader::signal_action_open_user_menu() {
    return m_signal_action_open_user_menu;
}

void ChatMessageHeader::AddContent(Gtk::Widget *widget, bool prepend) {
    m_content_box->add(*widget);
    if (prepend)
        m_content_box->reorder_child(*widget, 1);
}
