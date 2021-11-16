#include "chatmessage.hpp"
#include "../abaddon.hpp"
#include "../util.hpp"
#include "lazyimage.hpp"
#include <unordered_map>

constexpr static int EmojiSize = 24; // settings eventually
constexpr static int AvatarSize = 32;
constexpr static int EmbedImageWidth = 400;
constexpr static int EmbedImageHeight = 300;
constexpr static int ThumbnailSize = 100;
constexpr static int StickerComponentSize = 160;

ChatMessageItemContainer::ChatMessageItemContainer()
    : m_main(Gtk::ORIENTATION_VERTICAL) {
    add(m_main);

    m_link_menu_copy = Gtk::manage(new Gtk::MenuItem("Copy Link"));
    m_link_menu_copy->signal_activate().connect(sigc::mem_fun(*this, &ChatMessageItemContainer::on_link_menu_copy));
    m_link_menu.append(*m_link_menu_copy);

    m_link_menu.show_all();
}

ChatMessageItemContainer *ChatMessageItemContainer::FromMessage(const Message &data) {
    auto *container = Gtk::manage(new ChatMessageItemContainer);
    container->ID = data.ID;
    container->ChannelID = data.ChannelID;

    if (data.Nonce.has_value())
        container->Nonce = *data.Nonce;

    if (data.Content.size() > 0 || data.Type != MessageType::DEFAULT) {
        container->m_text_component = container->CreateTextComponent(data);
        container->AttachEventHandlers(*container->m_text_component);
        container->m_main.add(*container->m_text_component);
    }

    if ((data.MessageReference.has_value() || data.Interaction.has_value()) && data.Type != MessageType::CHANNEL_FOLLOW_ADD) {
        auto *widget = container->CreateReplyComponent(data);
        if (widget != nullptr) {
            container->m_main.add(*widget);
            container->m_main.child_property_position(*widget) = 0; // eek
        }
    }

    // there should only ever be 1 embed (i think?)
    if (data.Embeds.size() == 1) {
        const auto &embed = data.Embeds[0];
        if (IsEmbedImageOnly(embed)) {
            auto *widget = container->CreateImageComponent(*embed.Thumbnail->ProxyURL, *embed.Thumbnail->URL, *embed.Thumbnail->Width, *embed.Thumbnail->Height);
            container->AttachEventHandlers(*widget);
            container->m_main.add(*widget);
        } else {
            container->m_embed_component = container->CreateEmbedComponent(embed);
            container->AttachEventHandlers(*container->m_embed_component);
            container->m_main.add(*container->m_embed_component);
        }
    }

    // i dont think attachments can be edited
    // also this can definitely be done much better holy shit
    for (const auto &a : data.Attachments) {
        if (IsURLViewableImage(a.ProxyURL) && a.Width.has_value() && a.Height.has_value()) {
            auto *widget = container->CreateImageComponent(a.ProxyURL, a.URL, *a.Width, *a.Height);
            container->m_main.add(*widget);
        } else {
            auto *widget = container->CreateAttachmentComponent(a);
            container->m_main.add(*widget);
        }
    }

    // only 1?
    /*
    DEPRECATED
    if (data.Stickers.has_value()) {
        const auto &sticker = data.Stickers.value()[0];
        // todo: lottie, proper apng
        if (sticker.FormatType == StickerFormatType::PNG || sticker.FormatType == StickerFormatType::APNG) {
            auto *widget = container->CreateStickerComponent(sticker);
            container->m_main->add(*widget);
        }
    }*/

    if (data.StickerItems.has_value()) {
        auto *widget = container->CreateStickersComponent(*data.StickerItems);
        container->m_main.add(*widget);
    }

    if (data.Reactions.has_value() && data.Reactions->size() > 0) {
        container->m_reactions_component = container->CreateReactionsComponent(data);
        container->m_main.add(*container->m_reactions_component);
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
        m_embed_component = CreateEmbedComponent(data->Embeds[0]);
        AttachEventHandlers(*m_embed_component);
        m_main.add(*m_embed_component);
    }
}

void ChatMessageItemContainer::UpdateReactions() {
    if (m_reactions_component != nullptr) {
        delete m_reactions_component;
        m_reactions_component = nullptr;
    }

    const auto data = Abaddon::Get().GetDiscordClient().GetMessage(ID);
    if (data->Reactions.has_value() && data->Reactions->size() > 0) {
        m_reactions_component = CreateReactionsComponent(*data);
        m_reactions_component->show_all();
        m_main.add(*m_reactions_component);
    }
}

void ChatMessageItemContainer::SetFailed() {
    if (m_text_component != nullptr) {
        m_text_component->get_style_context()->remove_class("pending");
        m_text_component->get_style_context()->add_class("failed");
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
        m_main.add(*m_attrib_label); // todo: maybe insert markup into existing text widget's buffer if the circumstances are right (or pack horizontally)
    }

    if (deleted)
        m_attrib_label->set_markup("<span color='#ff0000'>[deleted]</span>");
    else if (edited)
        m_attrib_label->set_markup("<span color='#999999'>[edited]</span>");
}

void ChatMessageItemContainer::AddClickHandler(Gtk::Widget *widget, std::string url) {
    // clang-format off
    widget->signal_button_press_event().connect([url](GdkEventButton *event) -> bool {
        if (event->type == GDK_BUTTON_PRESS && event->button == GDK_BUTTON_PRIMARY) {
            LaunchBrowser(url);
            return false;
        }
        return true;
    }, false);
    // clang-format on
}

Gtk::TextView *ChatMessageItemContainer::CreateTextComponent(const Message &data) {
    auto *tv = Gtk::manage(new Gtk::TextView);

    if (data.IsPending)
        tv->get_style_context()->add_class("pending");
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
        case MessageType::INLINE_REPLY:
            b->insert(s, data->Content);
            HandleUserMentions(b);
            HandleLinks(*tv);
            HandleChannelMentions(tv);
            HandleEmojis(*tv);
            break;
        case MessageType::USER_PREMIUM_GUILD_SUBSCRIPTION:
            b->insert_markup(s, "<span color='#999999'><i>[boosted server]</i></span>");
            break;
        case MessageType::GUILD_MEMBER_JOIN:
            b->insert_markup(s, "<span color='#999999'><i>[user joined]</i></span>");
            break;
        case MessageType::CHANNEL_PINNED_MESSAGE:
            b->insert_markup(s, "<span color='#999999'><i>[message pinned]</i></span>");
            break;
        case MessageType::APPLICATION_COMMAND: {
            if (data->Application.has_value()) {
                static const auto regex = Glib::Regex::create(R"(</(.*?):(\d+)>)");
                Glib::MatchInfo match;
                if (regex->match(data->Content, match)) {
                    const auto cmd = match.fetch(1);
                    const auto app = data->Application->Name;
                    b->insert_markup(s, "<i>used <span color='#697ec4'>" + cmd + "</span> with " + app + "</i>");
                }
            } else {
                b->insert(s, data->Content);
                HandleUserMentions(b);
                HandleLinks(*tv);
                HandleChannelMentions(tv);
                HandleEmojis(*tv);
            }
        } break;
        case MessageType::RECIPIENT_ADD: {
            const auto &adder = Abaddon::Get().GetDiscordClient().GetUser(data->Author.ID);
            const auto &added = data->Mentions[0];
            b->insert_markup(s, "<i><span color='#999999'><span color='#eeeeee'>" + adder->Username + "</span> added <span color='#eeeeee'>" + added.Username + "</span></span></i>");
        } break;
        case MessageType::RECIPIENT_REMOVE: {
            const auto &adder = Abaddon::Get().GetDiscordClient().GetUser(data->Author.ID);
            const auto &added = data->Mentions[0];
            if (adder->ID == added.ID)
                b->insert_markup(s, "<i><span color='#999999'><span color='#eeeeee'>" + adder->Username + "</span> left</span></i>");
            else
                b->insert_markup(s, "<i><span color='#999999'><span color='#eeeeee'>" + adder->Username + "</span> removed <span color='#eeeeee'>" + added.Username + "</span></span></i>");
        } break;
        case MessageType::CHANNEL_NAME_CHANGE: {
            const auto author = Abaddon::Get().GetDiscordClient().GetUser(data->Author.ID);
            b->insert_markup(s, "<i><span color='#999999'>" + author->GetEscapedBoldName() + " changed the name to <b>" + Glib::Markup::escape_text(data->Content) + "</b></span></i>");
        } break;
        case MessageType::CHANNEL_ICON_CHANGE: {
            const auto author = Abaddon::Get().GetDiscordClient().GetUser(data->Author.ID);
            b->insert_markup(s, "<i><span color='#999999'>" + author->GetEscapedBoldName() + " changed the channel icon</span></i>");
        } break;
        case MessageType::USER_PREMIUM_GUILD_SUBSCRIPTION_TIER_1:
        case MessageType::USER_PREMIUM_GUILD_SUBSCRIPTION_TIER_2:
        case MessageType::USER_PREMIUM_GUILD_SUBSCRIPTION_TIER_3: {
            const auto author = Abaddon::Get().GetDiscordClient().GetUser(data->Author.ID);
            const auto guild = Abaddon::Get().GetDiscordClient().GetGuild(*data->GuildID);
            b->insert_markup(s, "<i><span color='#999999'>" + author->GetEscapedBoldName() + " just boosted the server <b>" + Glib::Markup::escape_text(data->Content) + "</b> times! " +
                                    Glib::Markup::escape_text(guild->Name) + " has achieved <b>Level " + std::to_string(static_cast<int>(data->Type) - 8) + "!</b></span></i>"); // oo cheeky me !!!
        } break;
        case MessageType::CHANNEL_FOLLOW_ADD: {
            const auto author = Abaddon::Get().GetDiscordClient().GetUser(data->Author.ID);
            b->insert_markup(s, "<i><span color='#999999'>" + author->GetEscapedBoldName() + " has added <b>" + Glib::Markup::escape_text(data->Content) + "</b> to this channel. Its most important updates will show up here.</span></i>");
        } break;
        case MessageType::CALL: {
            b->insert_markup(s, "<span color='#999999'><i>[started a call]</i></span>");
        } break;
        case MessageType::GUILD_DISCOVERY_DISQUALIFIED: {
            b->insert_markup(s, "<i><span color='#999999'>This server has been removed from Server Discovery because it no longer passes all the requirements.</span></i>");
        } break;
        case MessageType::GUILD_DISCOVERY_REQUALIFIED: {
            b->insert_markup(s, "<i><span color='#999999'>This server is eligible for Server Discovery again and has been automatically relisted!</span></i>");
        } break;
        case MessageType::GUILD_DISCOVERY_GRACE_PERIOD_INITIAL_WARNING: {
            b->insert_markup(s, "<i><span color='#999999'>This server has failed Discovery activity requirements for 1 week. If this server fails for 4 weeks in a row, it will be automatically removed from Discovery.</span></i>");
        } break;
        case MessageType::GUILD_DISCOVERY_GRACE_PERIOD_FINAL_WARNING: {
            b->insert_markup(s, "<i><span color='#999999'>This server has failed Discovery activity requirements for 3 weeks in a row. If this server fails for 1 more week, it will be removed from Discovery.</span></i>");
        } break;
        case MessageType::THREAD_CREATED: {
            const auto author = Abaddon::Get().GetDiscordClient().GetUser(data->Author.ID);
            if (data->MessageReference.has_value() && data->MessageReference->ChannelID.has_value()) {
                auto iter = b->insert_markup(s, "<i><span color='#999999'>" + author->GetEscapedBoldName() + " started a thread: </span></i>");
                auto tag = b->create_tag();
                tag->property_weight() = Pango::WEIGHT_BOLD;
                m_channel_tagmap[tag] = *data->MessageReference->ChannelID;
                b->insert_with_tag(iter, data->Content, tag);

                tv->signal_button_press_event().connect(sigc::mem_fun(*this, &ChatMessageItemContainer::OnClickChannel), false);
            } else {
                b->insert_markup(s, "<i><span color='#999999'>" + author->GetEscapedBoldName() + " started a thread: </span><b>" + Glib::Markup::escape_text(data->Content) + "</b></i>");
            }
        } break;
        default: break;
    }
}

Gtk::Widget *ChatMessageItemContainer::CreateEmbedComponent(const EmbedData &embed) {
    Gtk::EventBox *ev = Gtk::manage(new Gtk::EventBox);
    ev->set_can_focus(true);
    Gtk::Box *main = Gtk::manage(new Gtk::Box(Gtk::ORIENTATION_HORIZONTAL));
    Gtk::Box *content = Gtk::manage(new Gtk::Box(Gtk::ORIENTATION_VERTICAL));

    if (embed.Author.has_value() && (embed.Author->Name.has_value() || embed.Author->ProxyIconURL.has_value())) {
        auto *author_box = Gtk::manage(new Gtk::Box(Gtk::ORIENTATION_HORIZONTAL));
        content->pack_start(*author_box);

        constexpr static int AuthorIconSize = 20;
        if (embed.Author->ProxyIconURL.has_value()) {
            auto *author_img = Gtk::manage(new LazyImage(*embed.Author->ProxyIconURL, AuthorIconSize, AuthorIconSize));
            author_img->set_halign(Gtk::ALIGN_START);
            author_img->set_valign(Gtk::ALIGN_START);
            author_img->set_margin_start(6);
            author_img->set_margin_end(6);
            author_img->get_style_context()->add_class("embed-author-icon");
            author_box->add(*author_img);
        }

        if (embed.Author->Name.has_value()) {
            auto *author_lbl = Gtk::manage(new Gtk::Label);
            author_lbl->set_halign(Gtk::ALIGN_START);
            author_lbl->set_valign(Gtk::ALIGN_CENTER);
            author_lbl->set_line_wrap(true);
            author_lbl->set_line_wrap_mode(Pango::WRAP_WORD_CHAR);
            author_lbl->set_hexpand(false);
            author_lbl->set_text(*embed.Author->Name);
            author_lbl->get_style_context()->add_class("embed-author");
            author_box->add(*author_lbl);
        }
    }

    if (embed.Title.has_value()) {
        auto *title_ev = Gtk::manage(new Gtk::EventBox);
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
        title_ev->add(*title_label);
        content->pack_start(*title_ev);

        if (embed.URL.has_value()) {
            AddPointerCursor(*title_ev);
            auto url = *embed.URL;
            title_ev->signal_button_press_event().connect([this, url = std::move(url)](GdkEventButton *event) -> bool {
                if (event->button == GDK_BUTTON_PRIMARY) {
                    LaunchBrowser(url);
                    return true;
                }
                return false;
            });
            static auto color = Abaddon::Get().GetSettings().GetLinkColor();
            title_label->override_color(Gdk::RGBA(color));
            title_label->set_markup("<b>" + Glib::Markup::escape_text(*embed.Title) + "</b>");
        }
    }

    if (!embed.Provider.has_value() || embed.Provider->Name != "YouTube") { // youtube link = no description
        if (embed.Description.has_value()) {
            auto *desc_label = Gtk::manage(new Gtk::Label);
            desc_label->set_text(*embed.Description);
            desc_label->set_line_wrap(true);
            desc_label->set_line_wrap_mode(Pango::WRAP_WORD_CHAR);
            desc_label->set_max_width_chars(50);
            desc_label->set_halign(Gtk::ALIGN_START);
            desc_label->set_hexpand(false);
            desc_label->get_style_context()->add_class("embed-description");
            content->pack_start(*desc_label);
        }
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
        content->pack_start(*flow);

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

    if (embed.Image.has_value() && embed.Image->ProxyURL.has_value()) {
        int w = 0, h = 0;
        GetImageDimensions(*embed.Image->Width, *embed.Image->Height, w, h, EmbedImageWidth, EmbedImageHeight);

        auto *img = Gtk::manage(new LazyImage(*embed.Image->ProxyURL, w, h, false));
        img->set_halign(Gtk::ALIGN_CENTER);
        img->set_margin_top(5);
        img->set_size_request(w, h);
        content->pack_start(*img);
    }

    if (embed.Footer.has_value()) {
        auto *footer_lbl = Gtk::manage(new Gtk::Label);
        footer_lbl->set_halign(Gtk::ALIGN_START);
        footer_lbl->set_line_wrap(true);
        footer_lbl->set_line_wrap_mode(Pango::WRAP_WORD_CHAR);
        footer_lbl->set_hexpand(false);
        footer_lbl->set_text(embed.Footer->Text);
        footer_lbl->get_style_context()->add_class("embed-footer");
        content->pack_start(*footer_lbl);
    }

    if (embed.Thumbnail.has_value() && embed.Thumbnail->ProxyURL.has_value()) {
        int w, h;
        GetImageDimensions(*embed.Thumbnail->Width, *embed.Thumbnail->Height, w, h, ThumbnailSize, ThumbnailSize);

        auto *thumbnail = Gtk::manage(new LazyImage(*embed.Thumbnail->ProxyURL, w, h, false));
        thumbnail->set_size_request(w, h);
        thumbnail->set_margin_start(8);
        main->pack_end(*thumbnail);
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
    main->pack_start(*content);

    ev->add(*main);
    ev->show_all();

    return ev;
}

Gtk::Widget *ChatMessageItemContainer::CreateImageComponent(const std::string &proxy_url, const std::string &url, int inw, int inh) {
    int w, h;
    GetImageDimensions(inw, inh, w, h);

    Gtk::EventBox *ev = Gtk::manage(new Gtk::EventBox);
    Gtk::Image *widget = Gtk::manage(new LazyImage(proxy_url, w, h, false));
    ev->add(*widget);
    widget->set_halign(Gtk::ALIGN_START);
    widget->set_size_request(w, h);

    AttachEventHandlers(*ev);
    AddClickHandler(ev, url);

    return ev;
}

Gtk::Widget *ChatMessageItemContainer::CreateAttachmentComponent(const AttachmentData &data) {
    auto *ev = Gtk::manage(new Gtk::EventBox);
    auto *btn = Gtk::manage(new Gtk::Label(data.Filename + " " + HumanReadableBytes(data.Bytes))); // Gtk::LinkButton flat out doesn't work :D
    ev->set_hexpand(false);
    ev->set_halign(Gtk::ALIGN_START);
    ev->get_style_context()->add_class("message-attachment-box");
    ev->add(*btn);

    AttachEventHandlers(*ev);
    AddClickHandler(ev, data.URL);

    return ev;
}

Gtk::Widget *ChatMessageItemContainer::CreateStickerComponentDeprecated(const StickerData &data) {
    auto *box = Gtk::manage(new Gtk::Box(Gtk::ORIENTATION_HORIZONTAL));
    auto *imgw = Gtk::manage(new Gtk::Image);
    box->add(*imgw);
    auto &img = Abaddon::Get().GetImageManager();

    if (data.FormatType == StickerFormatType::PNG || data.FormatType == StickerFormatType::APNG) {
        auto cb = [this, imgw](const Glib::RefPtr<Gdk::Pixbuf> &pixbuf) {
            imgw->property_pixbuf() = pixbuf;
        };
        img.LoadFromURL(data.GetURL(), sigc::track_obj(cb, *imgw));
    }

    AttachEventHandlers(*box);
    return box;
}

Gtk::Widget *ChatMessageItemContainer::CreateStickersComponent(const std::vector<StickerItem> &data) {
    auto *box = Gtk::manage(new Gtk::Box(Gtk::ORIENTATION_VERTICAL));

    for (const auto &sticker : data) {
        // no lottie
        if (sticker.FormatType != StickerFormatType::PNG && sticker.FormatType != StickerFormatType::APNG) continue;
        auto *ev = Gtk::manage(new Gtk::EventBox);
        auto *img = Gtk::manage(new LazyImage(sticker.GetURL(), StickerComponentSize, StickerComponentSize, false));
        img->set_size_request(StickerComponentSize, StickerComponentSize); // should this go in LazyImage ?
        img->show();
        ev->show();
        ev->add(*img);
        box->add(*ev);
    }

    box->show();

    AttachEventHandlers(*box);
    return box;
}

Gtk::Widget *ChatMessageItemContainer::CreateReactionsComponent(const Message &data) {
    auto *flow = Gtk::manage(new Gtk::FlowBox);
    flow->set_orientation(Gtk::ORIENTATION_HORIZONTAL);
    flow->set_min_children_per_line(5);
    flow->set_max_children_per_line(20);
    flow->set_halign(Gtk::ALIGN_START);
    flow->set_hexpand(false);
    flow->set_column_spacing(2);
    flow->set_selection_mode(Gtk::SELECTION_NONE);

    auto &imgr = Abaddon::Get().GetImageManager();
    auto &emojis = Abaddon::Get().GetEmojis();
    const auto &placeholder = imgr.GetPlaceholder(16);

    for (const auto &reaction : *data.Reactions) {
        auto *ev = Gtk::manage(new Gtk::EventBox);
        auto *box = Gtk::manage(new Gtk::Box(Gtk::ORIENTATION_HORIZONTAL));
        box->get_style_context()->add_class("reaction-box");
        ev->add(*box);
        flow->add(*ev);

        bool is_stock = !reaction.Emoji.ID.IsValid();

        bool has_reacted = reaction.HasReactedWith;
        if (has_reacted)
            box->get_style_context()->add_class("reacted");

        ev->signal_button_press_event().connect([this, has_reacted, is_stock, reaction](GdkEventButton *event) -> bool {
            if (event->type == GDK_BUTTON_PRESS && event->button == GDK_BUTTON_PRIMARY) {
                Glib::ustring param; // escaped in client
                if (is_stock)
                    param = reaction.Emoji.Name;
                else
                    param = std::to_string(reaction.Emoji.ID);
                if (has_reacted)
                    m_signal_action_reaction_remove.emit(param);
                else
                    m_signal_action_reaction_add.emit(param);
                return true;
            }
            return false;
        });

        ev->signal_realize().connect([ev]() {
            auto window = ev->get_window();
            auto display = window->get_display();
            auto cursor = Gdk::Cursor::create(display, "pointer");
            window->set_cursor(cursor);
        });

        // image
        if (is_stock) { // unicode/stock
            const auto shortcode = emojis.GetShortCodeForPattern(reaction.Emoji.Name);
            if (shortcode != "")
                ev->set_tooltip_text(shortcode);

            const auto &pb = emojis.GetPixBuf(reaction.Emoji.Name);
            Gtk::Image *img;
            if (pb)
                img = Gtk::manage(new Gtk::Image(pb->scale_simple(16, 16, Gdk::INTERP_BILINEAR)));
            else
                img = Gtk::manage(new Gtk::Image(placeholder));
            img->set_can_focus(false);
            box->add(*img);
        } else { // custom
            ev->set_tooltip_text(reaction.Emoji.Name);

            auto img = Gtk::manage(new LazyImage(reaction.Emoji.GetURL(), 16, 16));
            img->set_can_focus(false);
            box->add(*img);
        }

        auto *lbl = Gtk::manage(new Gtk::Label(std::to_string(reaction.Count)));
        lbl->set_margin_left(5);
        lbl->get_style_context()->add_class("reaction-count");
        box->add(*lbl);
    }

    return flow;
}

Gtk::Widget *ChatMessageItemContainer::CreateReplyComponent(const Message &data) {
    if (data.Type == MessageType::THREAD_CREATED) return nullptr;

    auto *box = Gtk::manage(new Gtk::Box);
    auto *lbl = Gtk::manage(new Gtk::Label);
    lbl->set_single_line_mode(true);
    lbl->set_line_wrap(false);
    lbl->set_use_markup(true);
    lbl->set_ellipsize(Pango::ELLIPSIZE_END);
    lbl->get_style_context()->add_class("message-text"); // good idea?
    lbl->get_style_context()->add_class("message-reply");
    box->add(*lbl);

    const auto &discord = Abaddon::Get().GetDiscordClient();

    const auto get_author_markup = [&](Snowflake author_id, Snowflake guild_id = Snowflake::Invalid) -> std::string {
        if (guild_id.IsValid()) {
            const auto role_id = discord.GetMemberHoistedRole(guild_id, author_id, true);
            if (role_id.IsValid()) {
                const auto role = discord.GetRole(role_id);
                if (role.has_value()) {
                    const auto author = discord.GetUser(author_id);
                    return "<b><span color=\"#" + IntToCSSColor(role->Color) + "\">" + author->GetEscapedString() + "</span></b>";
                }
            }
        }

        const auto author = discord.GetUser(author_id);
        return author->GetEscapedBoldString<false>();
    };

    if (data.Interaction.has_value()) {
        const auto user = *discord.GetUser(data.Interaction->User.ID);

        if (data.GuildID.has_value()) {
            lbl->set_markup(get_author_markup(user.ID, *data.GuildID) +
                            " used <span color='#697ec4'>/" +
                            Glib::Markup::escape_text(data.Interaction->Name) +
                            "</span>");
        } else {
            lbl->set_markup(user.GetEscapedBoldString<false>());
        }
    } else if (data.ReferencedMessage.has_value()) {
        if (data.ReferencedMessage.value().get() == nullptr) {
            lbl->set_markup("<i>deleted message</i>");
        } else {
            const auto &referenced = *data.ReferencedMessage.value().get();
            Glib::ustring text;
            if (referenced.Content == "") {
                if (referenced.Attachments.size() > 0) {
                    text = "<i>attachment</i>";
                } else if (referenced.Embeds.size() > 0) {
                    text = "<i>embed</i>";
                }
            } else {
                auto buf = Gtk::TextBuffer::create();
                Gtk::TextBuffer::iterator start, end;
                buf->get_bounds(start, end);
                buf->set_text(referenced.Content);
                CleanupEmojis(buf);
                HandleUserMentions(buf);
                HandleChannelMentions(buf);
                text = Glib::Markup::escape_text(buf->get_text());
            }
            // getting markup out of a textbuffer seems like something that to me should be really simple
            // but actually is horribly annoying. replies won't have mention colors because you can't do this
            // also no emojis because idk how to make a textview act like a label
            // which of course would not be an issue if i could figure out how to get fonts to work on this god-forsaken framework
            // oh well
            // but ill manually get colors for the user who is being replied to
            if (referenced.GuildID.has_value())
                lbl->set_markup(get_author_markup(referenced.Author.ID, *referenced.GuildID) + ": " + text);
            else
                lbl->set_markup(get_author_markup(referenced.Author.ID) + ": " + text);
        }
    } else {
        lbl->set_markup("<i>reply unavailable</i>");
    }

    return box;
}

Glib::ustring ChatMessageItemContainer::GetText(const Glib::RefPtr<Gtk::TextBuffer> &buf) {
    Gtk::TextBuffer::iterator a, b;
    buf->get_bounds(a, b);
    auto slice = buf->get_slice(a, b, true);
    return slice;
}

bool ChatMessageItemContainer::IsEmbedImageOnly(const EmbedData &data) {
    if (!data.Thumbnail.has_value()) return false;
    if (data.Author.has_value()) return false;
    if (data.Description.has_value()) return false;
    if (data.Fields.has_value()) return false;
    if (data.Footer.has_value()) return false;
    if (data.Image.has_value()) return false;
    if (data.Timestamp.has_value()) return false;
    return data.Thumbnail->ProxyURL.has_value() && data.Thumbnail->URL.has_value() && data.Thumbnail->Width.has_value() && data.Thumbnail->Height.has_value();
}

void ChatMessageItemContainer::HandleUserMentions(Glib::RefPtr<Gtk::TextBuffer> buf) {
    constexpr static const auto mentions_regex = R"(<@!?(\d+)>)";

    static auto rgx = Glib::Regex::create(mentions_regex);

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
            replacement = user->GetEscapedBoldString<true>();
        else {
            const auto role_id = user->GetHoistedRole(*channel->GuildID, true);
            const auto role = discord.GetRole(role_id);
            if (!role.has_value())
                replacement = user->GetEscapedBoldString<true>();
            else
                replacement = "<span color=\"#" + IntToCSSColor(role->Color) + "\">" + user->GetEscapedBoldString<true>() + "</span>";
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

void ChatMessageItemContainer::HandleStockEmojis(Gtk::TextView &tv) {
    Abaddon::Get().GetEmojis().ReplaceEmojis(tv.get_buffer(), EmojiSize);
}

void ChatMessageItemContainer::HandleCustomEmojis(Gtk::TextView &tv) {
    static auto rgx = Glib::Regex::create(R"(<a?:([\w\d_]+):(\d+)>)");

    auto &img = Abaddon::Get().GetImageManager();

    auto buf = tv.get_buffer();
    auto text = GetText(buf);

    Glib::MatchInfo match;
    int startpos = 0;
    while (rgx->match(text, startpos, match)) {
        int mstart, mend;
        if (!match.fetch_pos(0, mstart, mend)) break;
        const bool is_animated = match.fetch(0)[1] == 'a';
        const bool show_animations = Abaddon::Get().GetSettings().GetShowAnimations();

        const auto chars_start = g_utf8_pointer_to_offset(text.c_str(), text.c_str() + mstart);
        const auto chars_end = g_utf8_pointer_to_offset(text.c_str(), text.c_str() + mend);
        auto start_it = buf->get_iter_at_offset(chars_start);
        auto end_it = buf->get_iter_at_offset(chars_end);

        startpos = mend;
        if (is_animated && show_animations) {
            const auto mark_start = buf->create_mark(start_it, false);
            end_it.backward_char();
            const auto mark_end = buf->create_mark(end_it, false);
            const auto cb = [this, &tv, buf, mark_start, mark_end](const Glib::RefPtr<Gdk::PixbufAnimation> &pixbuf) {
                auto start_it = mark_start->get_iter();
                auto end_it = mark_end->get_iter();
                end_it.forward_char();
                buf->delete_mark(mark_start);
                buf->delete_mark(mark_end);
                auto it = buf->erase(start_it, end_it);
                const auto anchor = buf->create_child_anchor(it);
                auto img = Gtk::manage(new Gtk::Image(pixbuf));
                img->show();
                tv.add_child_at_anchor(*img, anchor);
            };
            img.LoadAnimationFromURL(EmojiData::URLFromID(match.fetch(2), "gif"), EmojiSize, EmojiSize, sigc::track_obj(cb, tv));
        } else {
            // can't erase before pixbuf is ready or else marks that are in the same pos get mixed up
            const auto mark_start = buf->create_mark(start_it, false);
            end_it.backward_char();
            const auto mark_end = buf->create_mark(end_it, false);
            const auto cb = [this, buf, mark_start, mark_end](const Glib::RefPtr<Gdk::Pixbuf> &pixbuf) {
                auto start_it = mark_start->get_iter();
                auto end_it = mark_end->get_iter();
                end_it.forward_char();
                buf->delete_mark(mark_start);
                buf->delete_mark(mark_end);
                auto it = buf->erase(start_it, end_it);
                buf->insert_pixbuf(it, pixbuf->scale_simple(EmojiSize, EmojiSize, Gdk::INTERP_BILINEAR));
            };
            img.LoadFromURL(EmojiData::URLFromID(match.fetch(2)), sigc::track_obj(cb, tv));
        }

        text = GetText(buf);
    }
}

void ChatMessageItemContainer::HandleEmojis(Gtk::TextView &tv) {
    static const bool stock_emojis = Abaddon::Get().GetSettings().GetShowStockEmojis();
    static const bool custom_emojis = Abaddon::Get().GetSettings().GetShowCustomEmojis();

    if (stock_emojis) HandleStockEmojis(tv);
    if (custom_emojis) HandleCustomEmojis(tv);
}

void ChatMessageItemContainer::CleanupEmojis(Glib::RefPtr<Gtk::TextBuffer> buf) {
    static auto rgx = Glib::Regex::create(R"(<a?:([\w\d_]+):(\d+)>)");

    auto text = GetText(buf);

    Glib::MatchInfo match;
    int startpos = 0;
    while (rgx->match(text, startpos, match)) {
        int mstart, mend;
        if (!match.fetch_pos(0, mstart, mend)) break;

        const auto new_term = ":" + match.fetch(1) + ":";

        const auto chars_start = g_utf8_pointer_to_offset(text.c_str(), text.c_str() + mstart);
        const auto chars_end = g_utf8_pointer_to_offset(text.c_str(), text.c_str() + mend);
        auto start_it = buf->get_iter_at_offset(chars_start);
        auto end_it = buf->get_iter_at_offset(chars_end);

        startpos = mend;
        const auto it = buf->erase(start_it, end_it);
        const int alen = text.size();
        text = GetText(buf);
        const int blen = text.size();
        startpos -= (alen - blen);

        buf->insert(it, new_term);

        text = GetText(buf);
    }
}

void ChatMessageItemContainer::HandleChannelMentions(Glib::RefPtr<Gtk::TextBuffer> buf) {
    static auto rgx = Glib::Regex::create(R"(<#(\d+)>)");

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
        if (chan->Type == ChannelType::GUILD_TEXT) {
            m_channel_tagmap[tag] = channel_id;
            tag->property_weight() = Pango::WEIGHT_BOLD;
        }

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

void ChatMessageItemContainer::HandleChannelMentions(Gtk::TextView *tv) {
    tv->signal_button_press_event().connect(sigc::mem_fun(*this, &ChatMessageItemContainer::OnClickChannel), false);
    HandleChannelMentions(tv->get_buffer());
}

// a lot of repetition here so there should probably just be one slot for textview's button-press
bool ChatMessageItemContainer::OnClickChannel(GdkEventButton *ev) {
    if (m_text_component == nullptr) return false;
    if (ev->type != GDK_BUTTON_PRESS) return false;
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

void ChatMessageItemContainer::HandleLinks(Gtk::TextView &tv) {
    const auto rgx = Glib::Regex::create(R"(\bhttps?:\/\/[^\s]+\.[^\s]+\b)");

    tv.signal_button_press_event().connect(sigc::mem_fun(*this, &ChatMessageItemContainer::OnLinkClick), false);

    auto buf = tv.get_buffer();
    Glib::ustring text = GetText(buf);

    // i'd like to let this be done thru css like .message-link { color: #bitch; } but idk how
    static auto link_color = Abaddon::Get().GetSettings().GetLinkColor();

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
    if (ev->type != GDK_BUTTON_PRESS) return false;
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
}

ChatMessageItemContainer::type_signal_channel_click ChatMessageItemContainer::signal_action_channel_click() {
    return m_signal_action_channel_click;
}

ChatMessageItemContainer::type_signal_action_reaction_add ChatMessageItemContainer::signal_action_reaction_add() {
    return m_signal_action_reaction_add;
}

ChatMessageItemContainer::type_signal_action_reaction_remove ChatMessageItemContainer::signal_action_reaction_remove() {
    return m_signal_action_reaction_remove;
}

void ChatMessageItemContainer::AttachEventHandlers(Gtk::Widget &widget) {
    const auto on_button_press_event = [this](GdkEventButton *e) -> bool {
        if (e->type == GDK_BUTTON_PRESS && e->button == GDK_BUTTON_SECONDARY) {
            event(reinterpret_cast<GdkEvent *>(e)); // illegal ooooooh
            return true;
        }

        return false;
    };
    widget.signal_button_press_event().connect(on_button_press_event, false);
}

ChatMessageHeader::ChatMessageHeader(const Message &data)
    : m_main_box(Gtk::ORIENTATION_HORIZONTAL)
    , m_content_box(Gtk::ORIENTATION_VERTICAL)
    , m_meta_box(Gtk::ORIENTATION_HORIZONTAL)
    , m_avatar(Abaddon::Get().GetImageManager().GetPlaceholder(AvatarSize)) {
    UserID = data.Author.ID;
    ChannelID = data.ChannelID;

    const auto author = Abaddon::Get().GetDiscordClient().GetUser(UserID);
    auto &img = Abaddon::Get().GetImageManager();

    auto cb = [this](const Glib::RefPtr<Gdk::Pixbuf> &pb) {
        m_static_avatar = pb->scale_simple(AvatarSize, AvatarSize, Gdk::INTERP_BILINEAR);
        m_avatar.property_pixbuf() = m_static_avatar;
    };
    img.LoadFromURL(author->GetAvatarURL(data.GuildID), sigc::track_obj(cb, *this));

    if (author->HasAnimatedAvatar()) {
        auto cb = [this](const Glib::RefPtr<Gdk::PixbufAnimation> &pb) {
            m_anim_avatar = pb;
        };
        img.LoadAnimationFromURL(author->GetAvatarURL("gif"), AvatarSize, AvatarSize, sigc::track_obj(cb, *this));
    }

    get_style_context()->add_class("message-container");
    m_author.get_style_context()->add_class("message-container-author");
    m_timestamp.get_style_context()->add_class("message-container-timestamp");
    m_avatar.get_style_context()->add_class("message-container-avatar");

    m_avatar.set_valign(Gtk::ALIGN_START);
    m_avatar.set_margin_right(10);

    m_author.set_markup(data.Author.GetEscapedBoldName());
    m_author.set_single_line_mode(true);
    m_author.set_line_wrap(false);
    m_author.set_ellipsize(Pango::ELLIPSIZE_END);
    m_author.set_xalign(0.f);
    m_author.set_can_focus(false);

    m_meta_ev.signal_button_press_event().connect(sigc::mem_fun(*this, &ChatMessageHeader::on_author_button_press));

    if (author->IsBot || data.WebhookID.has_value()) {
        m_extra = Gtk::manage(new Gtk::Label);
        m_extra->get_style_context()->add_class("message-container-extra");
        m_extra->set_single_line_mode(true);
        m_extra->set_margin_start(12);
        m_extra->set_can_focus(false);
        m_extra->set_use_markup(true);
    }
    if (author->IsBot)
        m_extra->set_markup("<b>BOT</b>");
    else if (data.WebhookID.has_value())
        m_extra->set_markup("<b>Webhook</b>");

    m_timestamp.set_text(data.ID.GetLocalTimestamp());
    m_timestamp.set_hexpand(true);
    m_timestamp.set_halign(Gtk::ALIGN_END);
    m_timestamp.set_ellipsize(Pango::ELLIPSIZE_END);
    m_timestamp.set_opacity(0.5);
    m_timestamp.set_single_line_mode(true);
    m_timestamp.set_margin_start(12);
    m_timestamp.set_can_focus(false);

    m_main_box.set_hexpand(true);
    m_main_box.set_vexpand(true);
    m_main_box.set_can_focus(true);

    m_meta_box.set_hexpand(true);
    m_meta_box.set_can_focus(false);

    m_content_box.set_can_focus(false);

    const auto on_enter_cb = [this](const GdkEventCrossing *event) -> bool {
        if (m_anim_avatar)
            m_avatar.property_pixbuf_animation() = m_anim_avatar;
        return false;
    };
    const auto on_leave_cb = [this](const GdkEventCrossing *event) -> bool {
        if (m_anim_avatar)
            m_avatar.property_pixbuf() = m_static_avatar;
        return false;
    };

    m_content_box_ev.add_events(Gdk::ENTER_NOTIFY_MASK | Gdk::LEAVE_NOTIFY_MASK);
    m_meta_ev.add_events(Gdk::ENTER_NOTIFY_MASK | Gdk::LEAVE_NOTIFY_MASK);
    m_avatar_ev.add_events(Gdk::ENTER_NOTIFY_MASK | Gdk::LEAVE_NOTIFY_MASK);
    if (Abaddon::Get().GetSettings().GetShowAnimations()) {
        m_content_box_ev.signal_enter_notify_event().connect(on_enter_cb);
        m_content_box_ev.signal_leave_notify_event().connect(on_leave_cb);
        m_meta_ev.signal_enter_notify_event().connect(on_enter_cb);
        m_meta_ev.signal_leave_notify_event().connect(on_leave_cb);
        m_avatar_ev.signal_enter_notify_event().connect(on_enter_cb);
        m_avatar_ev.signal_leave_notify_event().connect(on_leave_cb);
    }

    m_meta_box.add(m_author);
    if (m_extra != nullptr)
        m_meta_box.add(*m_extra);

    m_meta_box.add(m_timestamp);
    m_meta_ev.add(m_meta_box);
    m_content_box.add(m_meta_ev);
    m_avatar_ev.add(m_avatar);
    m_main_box.add(m_avatar_ev);
    m_content_box_ev.add(m_content_box);
    m_main_box.add(m_content_box_ev);
    add(m_main_box);

    set_margin_bottom(8);

    show_all();

    auto &discord = Abaddon::Get().GetDiscordClient();
    auto role_update_cb = [this](...) { UpdateNameColor(); };
    discord.signal_role_update().connect(sigc::track_obj(role_update_cb, *this));
    auto guild_member_update_cb = [this](const auto &, const auto &) { UpdateNameColor(); };
    discord.signal_guild_member_update().connect(sigc::track_obj(guild_member_update_cb, *this));
    UpdateNameColor();
    AttachUserMenuHandler(m_meta_ev);
    AttachUserMenuHandler(m_avatar_ev);
}

void ChatMessageHeader::UpdateNameColor() {
    const auto &discord = Abaddon::Get().GetDiscordClient();
    const auto user = discord.GetUser(UserID);
    if (!user.has_value()) return;
    const auto chan = discord.GetChannel(ChannelID);
    bool is_guild = chan.has_value() && chan->GuildID.has_value();
    if (is_guild) {
        const auto role_id = discord.GetMemberHoistedRole(*chan->GuildID, UserID, true);
        const auto role = discord.GetRole(role_id);

        std::string md;
        if (role.has_value())
            m_author.set_markup("<span weight='bold' color='#" + IntToCSSColor(role->Color) + "'>" + user->GetEscapedName() + "</span>");
        else
            m_author.set_markup("<span weight='bold'>" + user->GetEscapedName() + "</span>");
    } else
        m_author.set_markup("<span weight='bold'>" + user->GetEscapedName() + "</span>");
}

std::vector<Gtk::Widget *> ChatMessageHeader::GetChildContent() {
    return m_content_widgets;
}

void ChatMessageHeader::AttachUserMenuHandler(Gtk::Widget &widget) {
    widget.signal_button_press_event().connect([this](GdkEventButton *ev) -> bool {
        if (ev->type == GDK_BUTTON_PRESS && ev->button == GDK_BUTTON_SECONDARY) {
            auto info = Abaddon::Get().GetDiscordClient().GetChannel(ChannelID);
            Snowflake guild_id;
            if (info.has_value() && info->GuildID.has_value())
                guild_id = *info->GuildID;
            Abaddon::Get().ShowUserMenu(reinterpret_cast<GdkEvent *>(ev), UserID, guild_id);
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
    m_content_widgets.push_back(widget);
    const auto cb = [this, widget]() {
        m_content_widgets.erase(std::remove(m_content_widgets.begin(), m_content_widgets.end(), widget), m_content_widgets.end());
    };
    widget->signal_unmap().connect(sigc::track_obj(cb, *this, *widget), false);
    m_content_box.add(*widget);
    if (prepend)
        m_content_box.reorder_child(*widget, 1);
    if (auto *x = dynamic_cast<ChatMessageItemContainer *>(widget)) {
        if (x->ID > NewestID)
            NewestID = x->ID;
    }
}
