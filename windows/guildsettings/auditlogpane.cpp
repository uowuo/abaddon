#include "auditlogpane.hpp"
#include "../../abaddon.hpp"

using namespace std::string_literals;

GuildSettingsAuditLogPane::GuildSettingsAuditLogPane(Snowflake id)
    : GuildID(id) {
    set_name("guild-audit-log-pane");
    set_hexpand(true);
    set_vexpand(true);

    auto &discord = Abaddon::Get().GetDiscordClient();
    const auto self_id = discord.GetUserData().ID;

    if (discord.HasGuildPermission(self_id, GuildID, Permission::VIEW_AUDIT_LOG))
        discord.FetchAuditLog(GuildID, sigc::mem_fun(*this, &GuildSettingsAuditLogPane::OnAuditLogFetch));

    m_list.set_selection_mode(Gtk::SELECTION_NONE);
    m_list.show();
    add(m_list);
}

void GuildSettingsAuditLogPane::OnAuditLogFetch(const AuditLogData &data) {
    auto &discord = Abaddon::Get().GetDiscordClient();
    auto guild = *discord.GetGuild(GuildID);
    for (const auto &entry : data.Entries) {
        auto expander = Gtk::manage(new Gtk::Expander);
        auto label = Gtk::manage(new Gtk::Label);
        label->set_ellipsize(Pango::ELLIPSIZE_END);

        auto user = *discord.GetUser(entry.UserID);

        // spaghetti moment
        Glib::ustring markup;
        std::vector<Glib::ustring> extra_markup;
        switch (entry.Type) {
            case AuditLogActionType::GUILD_UPDATE: {
                markup =
                    "<b>" + user.GetEscapedString() + "</b> " +
                    "made changes to <b>" +
                    Glib::Markup::escape_text(guild.Name) +
                    "</b>";

                if (entry.Changes.has_value())
                    for (const auto &change : *entry.Changes) {
                        if (change.Key == "icon_hash") {
                            extra_markup.push_back("Set the server icon");
                        } else if (change.Key == "name") {
                            auto new_name = change.NewValue;
                            if (new_name.has_value())
                                extra_markup.push_back("Set the server name to <b>" +
                                                       Glib::Markup::escape_text(new_name->get<std::string>()) +
                                                       "</b>");
                            else
                                extra_markup.push_back("Set the server name");
                        }
                    }
            } break;
            case AuditLogActionType::CHANNEL_CREATE: {
                const auto type = *entry.GetNewFromKey<ChannelType>("type");
                markup = "<b>" + user.GetEscapedString() + "</b> " +
                         "created a " + (type == ChannelType::GUILD_VOICE ? "voice" : "text") +
                         " channel <b>#" +
                         Glib::Markup::escape_text(*entry.GetNewFromKey<std::string>("name")) +
                         "</b>";
                if (entry.Changes.has_value())
                    for (const auto &change : *entry.Changes) {
                        if (change.Key == "name" && change.NewValue.has_value())
                            extra_markup.push_back("Set the name to <b>" +
                                                   Glib::Markup::escape_text(change.NewValue->get<std::string>()) +
                                                   "</b>");
                        else if (change.Key == "nsfw" && change.NewValue.has_value())
                            extra_markup.push_back((*change.NewValue ? "Marked" : "Unmarked") +
                                                   " the channel as NSFW"s);
                    }

            } break;
            case AuditLogActionType::CHANNEL_UPDATE: {
                const auto target_channel = discord.GetChannel(entry.TargetID);
                if (target_channel.has_value()) {
                    markup = "<b>" + user.GetEscapedString() + "</b> " +
                             "made changes to <b>#" +
                             Glib::Markup::escape_text(*target_channel->Name) +
                             "</b>";
                } else {
                    markup = "<b>" + user.GetEscapedString() + "</b> " +
                             "made changes to <b>&lt;#" +
                             entry.TargetID +
                             "&gt;</b>";
                }
                if (entry.Changes.has_value())
                    for (const auto &change : *entry.Changes) {
                        if (change.Key == "name" && change.NewValue.has_value()) {
                            if (change.OldValue.has_value())
                                extra_markup.push_back("Changed the name from <b>" +
                                                       Glib::Markup::escape_text(change.OldValue->get<std::string>()) +
                                                       "</b> to <b>" +
                                                       Glib::Markup::escape_text(change.NewValue->get<std::string>()) +
                                                       "</b>");
                            else
                                extra_markup.push_back("Changed the name to <b>" +
                                                       Glib::Markup::escape_text(change.NewValue->get<std::string>()) +
                                                       "</b>");
                        } else if (change.Key == "topic") {
                            if (change.NewValue.has_value())
                                extra_markup.push_back("Changed the topic to <b>" +
                                                       Glib::Markup::escape_text(change.NewValue->get<std::string>()) +
                                                       "</b>");
                            else
                                extra_markup.push_back("Cleared the topic");
                        } else if (change.Key == "nsfw" && change.NewValue.has_value()) {
                            extra_markup.push_back((*change.NewValue ? "Marked" : "Unmarked") + " the channel as NSFW"s);
                        } else if (change.Key == "rate_limit_per_user" && change.NewValue.has_value()) {
                            const int secs = change.NewValue->get<int>();
                            if (secs == 0)
                                extra_markup.push_back("Disabled slowmode");
                            else
                                extra_markup.push_back("Set slowmode to <b>" +
                                                       std::to_string(secs) + " seconds</b>");
                        }
                    }
            } break;
            case AuditLogActionType::CHANNEL_DELETE: {
                markup = "<b>" + user.GetEscapedString() + "</b> " +
                         "removed <b>#" +
                         Glib::Markup::escape_text(*entry.GetOldFromKey<std::string>("name")) +
                         "</b>";
            } break;
            case AuditLogActionType::CHANNEL_OVERWRITE_CREATE: {
                const auto channel = discord.GetChannel(entry.TargetID);
                if (channel.has_value()) {
                    markup = "<b>" + user.GetEscapedString() + "</b> " +
                             "created channel overrides for <b>#" +
                             Glib::Markup::escape_text(*channel->Name) + "</b>";
                } else {
                    markup = "<b>" + user.GetEscapedString() + "</b> " +
                             "created channel overrides for <b>&lt;#" +
                             entry.TargetID + "&gt;</b>";
                }
            } break;
            case AuditLogActionType::CHANNEL_OVERWRITE_UPDATE: {
                const auto channel = discord.GetChannel(entry.TargetID);
                if (channel.has_value()) {
                    markup = "<b>" + user.GetEscapedString() + "</b> " +
                             "updated channel overrides for <b>#" +
                             Glib::Markup::escape_text(*channel->Name) + "</b>";
                } else {
                    markup = "<b>" + user.GetEscapedString() + "</b> " +
                             "updated channel overrides for <b>&lt;#" +
                             entry.TargetID + "&gt;</b>";
                }
            } break;
            case AuditLogActionType::CHANNEL_OVERWRITE_DELETE: {
                const auto channel = discord.GetChannel(entry.TargetID);
                if (channel.has_value()) {
                    markup = "<b>" + user.GetEscapedString() + "</b> " +
                             "removed channel overrides for <b>#" +
                             Glib::Markup::escape_text(*channel->Name) + "</b>";
                } else {
                    markup = "<b>" + user.GetEscapedString() + "</b> " +
                             "removed channel overrides for <b>&lt;#" +
                             entry.TargetID + "&gt;</b>";
                }
            } break;
            case AuditLogActionType::MEMBER_KICK: {
                const auto target_user = discord.GetUser(entry.TargetID);
                markup = "<b>" + user.GetEscapedString() + "</b> " +
                         "kicked <b>" +
                         target_user->GetEscapedString() +
                         "</b>";
            } break;
            case AuditLogActionType::MEMBER_PRUNE: {
                markup = "<b>" + user.GetEscapedString() + "</b> " +
                         "pruned <b>" +
                         *entry.Options->MembersRemoved +
                         "</b> members";
                extra_markup.push_back("For <b>" +
                                       *entry.Options->DeleteMemberDays +
                                       " days</b> of inactivity");
            } break;
            case AuditLogActionType::MEMBER_BAN_ADD: {
                const auto target_user = discord.GetUser(entry.TargetID);
                markup = "<b>" + user.GetEscapedString() + "</b> " +
                         "banned <b>" +
                         target_user->GetEscapedString() +
                         "</b>";
            } break;
            case AuditLogActionType::MEMBER_BAN_REMOVE: {
                const auto target_user = discord.GetUser(entry.TargetID);
                markup = "<b>" + user.GetEscapedString() + "</b> " +
                         "removed the ban for <b>" +
                         target_user->GetEscapedString() +
                         "</b>";
            } break;
            case AuditLogActionType::MEMBER_UPDATE: {
                const auto target_user = discord.GetUser(entry.TargetID);
                markup = "<b>" + user.GetEscapedString() + "</b> " +
                         "updated <b>" +
                         target_user->GetEscapedString() +
                         "</b>";
                if (entry.Changes.has_value())
                    for (const auto &change : *entry.Changes) {
                        if (change.Key == "deaf" && change.NewValue.has_value())
                            extra_markup.push_back(
                                (change.NewValue->get<bool>() ? "<b>Deafened</b>"s : "<b>Undeafened</b>"s) +
                                " them");
                        else if (change.Key == "mute" && change.NewValue.has_value())
                            extra_markup.push_back(
                                (change.NewValue->get<bool>() ? "<b>Muted</b>"s : "<b>Unmuted</b>"s) +
                                " them");
                        else if (change.Key == "nick" && change.NewValue.has_value())
                            extra_markup.push_back("Set their nickname to <b>" +
                                                   Glib::Markup::escape_text(change.NewValue->get<std::string>()) +
                                                   "</b>");
                    }
            } break;
            case AuditLogActionType::MEMBER_ROLE_UPDATE: {
                const auto target_user = discord.GetUser(entry.TargetID);
                markup = "<b>" + user.GetEscapedString() + "</b> " +
                         "updated roles for <b>" +
                         target_user->GetEscapedString() + "</b>";
                if (entry.Changes.has_value())
                    for (const auto &change : *entry.Changes) {
                        if (change.Key == "$remove" && change.NewValue.has_value()) {
                            extra_markup.push_back("<b>Removed</b> a role <b>" +
                                                   Glib::Markup::escape_text(change.NewValue.value()[0].at("name").get<std::string>()) +
                                                   "</b>");
                        } else if (change.Key == "$add" && change.NewValue.has_value()) {
                            extra_markup.push_back("<b>Added</b> a role <b>" +
                                                   Glib::Markup::escape_text(change.NewValue.value()[0].at("name").get<std::string>()) +
                                                   "</b>");
                        }
                    }
            } break;
            case AuditLogActionType::MEMBER_MOVE: {
                const auto channel = discord.GetChannel(*entry.Options->ChannelID);
                markup = "<b>" + user.GetEscapedString() + "</b> " +
                         "moved <b>" +
                         *entry.Options->Count +
                         " user" +
                         (*entry.Options->Count == "1" ? ""s : "s"s) +
                         "</b> to <b>" +
                         Glib::Markup::escape_text(*channel->Name) +
                         "</b>";
            } break;
            case AuditLogActionType::MEMBER_DISCONNECT: {
                markup = "<b>" + user.GetEscapedString() + "</b> " +
                         "disconnected <b>" +
                         *entry.Options->Count +
                         "</b> users from voice";
            } break;
            case AuditLogActionType::BOT_ADD: {
                const auto target_user = discord.GetUser(entry.TargetID);
                markup = "<b>" + user.GetEscapedString() + "</b> " +
                         "added <b>" +
                         target_user->GetEscapedString() +
                         "</b> to the server";
            } break;
            case AuditLogActionType::ROLE_CREATE: {
                markup = "<b>" + user.GetEscapedString() + "</b> " +
                         "created the role <b>" +
                         *entry.GetNewFromKey<std::string>("name") +
                         "</b>";
            } break;
            case AuditLogActionType::ROLE_UPDATE: {
                const auto role = discord.GetRole(entry.TargetID);
                markup = "<b>" + user.GetEscapedString() + "</b> " +
                         "updated the role <b>" +
                         (role.has_value() ? Glib::Markup::escape_text(role->Name) : Glib::ustring(entry.TargetID)) +
                         "</b>";
                if (entry.Changes.has_value())
                    for (const auto &change : *entry.Changes) {
                        if (change.Key == "name" && change.NewValue.has_value()) {
                            extra_markup.push_back("Changed the name to <b>" +
                                                   Glib::Markup::escape_text(change.NewValue->get<std::string>()) +
                                                   "</b>");
                        } else if (change.Key == "color" && change.NewValue.has_value()) {
                            const auto col = change.NewValue->get<int>();
                            if (col == 0)
                                extra_markup.push_back("Removed the color");
                            else
                                extra_markup.push_back("Set the color to <b>" +
                                                       IntToCSSColor(col) +
                                                       "</b>");
                        } else if (change.Key == "permissions") {
                            extra_markup.push_back("Updated the permissions");
                        } else if (change.Key == "mentionable" && change.NewValue.has_value()) {
                            extra_markup.push_back(change.NewValue->get<bool>() ? "Mentionable" : "Not mentionable");
                        } else if (change.Key == "hoist" && change.NewValue.has_value()) {
                            extra_markup.push_back(change.NewValue->get<bool>() ? "Not hoisted" : "Hoisted");
                        }
                    }
            } break;
            case AuditLogActionType::ROLE_DELETE: {
                markup = "<b>" + user.GetEscapedString() + "</b> " +
                         "deleted the role <b>" +
                         *entry.GetOldFromKey<std::string>("name") +
                         "</b>";
            } break;
            case AuditLogActionType::INVITE_CREATE: {
                const auto code = *entry.GetNewFromKey<std::string>("code");
                markup = "<b>" + user.GetEscapedString() + "</b> " +
                         "created an invite <b>" + code + "</b>";
                if (entry.Changes.has_value())
                    for (const auto &change : *entry.Changes) {
                        if (change.Key == "channel_id" && change.NewValue.has_value()) {
                            const auto channel = discord.GetChannel(change.NewValue->get<Snowflake>());
                            if (!channel.has_value()) continue;
                            extra_markup.push_back("For channel <b>#" +
                                                   Glib::Markup::escape_text(*channel->Name) +
                                                   "</b>");
                        } else if (change.Key == "max_uses" && change.NewValue.has_value()) {
                            const auto uses = change.NewValue->get<int>();
                            if (uses == 0)
                                extra_markup.push_back("Which has <b>unlimited</b> uses");
                            else
                                extra_markup.push_back("Which has <b>" + std::to_string(uses) + "</b> uses");
                        } else if (change.Key == "temporary" && change.NewValue.has_value()) {
                            extra_markup.push_back("With temporary <b>"s +
                                                   (change.NewValue->get<bool>() ? "on" : "off") +
                                                   "</b>");
                        } // no max_age cuz fuck time
                    }
            } break;
            case AuditLogActionType::INVITE_DELETE: {
                markup = "<b>" + user.GetEscapedString() + "</b> " +
                         "deleted an invite <b>" +
                         *entry.GetOldFromKey<std::string>("code") +
                         "</b>";
            } break;
            case AuditLogActionType::WEBHOOK_CREATE: {
                markup = "<b>" + user.GetEscapedString() + "</b> " +
                         "created the webhook <b>" +
                         Glib::Markup::escape_text(*entry.GetNewFromKey<std::string>("name")) +
                         "</b>";
                for (const auto &change : *entry.Changes) {
                    if (change.Key == "channel_id" && change.NewValue.has_value()) {
                        const auto channel = discord.GetChannel(change.NewValue->get<Snowflake>());
                        if (channel.has_value()) {
                            extra_markup.push_back("With channel <b>#" +
                                                   Glib::Markup::escape_text(*channel->Name) +
                                                   "</b>");
                        }
                    }
                }
            } break;
            case AuditLogActionType::WEBHOOK_UPDATE: {
                const WebhookData *webhookptr = nullptr;
                for (const auto &webhook : data.Webhooks) {
                    if (webhook.ID == entry.TargetID)
                        webhookptr = &webhook;
                }
                if (webhookptr != nullptr) {
                    markup = "<b>" + user.GetEscapedString() + "</b> " +
                             "updated the webhook <b>" +
                             Glib::Markup::escape_text(webhookptr->Name) +
                             "</b>";
                } else {
                    markup = "<b>" + user.GetEscapedString() + "</b> " +
                             "updated a webhook";
                }
                if (entry.Changes.has_value())
                    for (const auto &change : *entry.Changes) {
                        if (change.Key == "name" && change.NewValue.has_value()) {
                            extra_markup.push_back("Changed the name to <b>" +
                                                   Glib::Markup::escape_text(change.NewValue->get<std::string>()) +
                                                   "</b>");
                        } else if (change.Key == "avatar_hash") {
                            extra_markup.push_back("Changed the avatar");
                        } else if (change.Key == "channel_id" && change.NewValue.has_value()) {
                            const auto channel = discord.GetChannel(change.NewValue->get<Snowflake>());
                            if (channel.has_value()) {
                                extra_markup.push_back("Changed the channel to <b>#" +
                                                       Glib::Markup::escape_text(*channel->Name) +
                                                       "</b>");
                            } else {
                                extra_markup.push_back("Changed the channel");
                            }
                        }
                    }
            } break;
            case AuditLogActionType::WEBHOOK_DELETE: {
                markup = "<b>" + user.GetEscapedString() + "</b> " +
                         "deleted the webhook <b>" +
                         Glib::Markup::escape_text(*entry.GetOldFromKey<std::string>("name")) +
                         "</b>";
            } break;
            case AuditLogActionType::EMOJI_CREATE: {
                markup = "<b>" + user.GetEscapedString() + "</b> " +
                         "created the emoji <b>" +
                         Glib::Markup::escape_text(*entry.GetNewFromKey<std::string>("name")) +
                         "</b>";
            } break;
            case AuditLogActionType::EMOJI_UPDATE: {
                markup = "<b>" + user.GetEscapedString() + "</b> " +
                         "updated the emoji <b>" +
                         Glib::Markup::escape_text(*entry.GetOldFromKey<std::string>("name")) +
                         "</b>";
                extra_markup.push_back("Changed the name from <b>" +
                                       Glib::Markup::escape_text(*entry.GetOldFromKey<std::string>("name")) +
                                       "</b> to <b>" +
                                       Glib::Markup::escape_text(*entry.GetNewFromKey<std::string>("name")) +
                                       "</b>");
            } break;
            case AuditLogActionType::EMOJI_DELETE: {
                markup = "<b>" + user.GetEscapedString() + "</b> " +
                         "deleted the emoji <b>" +
                         Glib::Markup::escape_text(*entry.GetOldFromKey<std::string>("name")) +
                         "</b>";
            } break;
            case AuditLogActionType::MESSAGE_PIN: {
                const auto target_user = discord.GetUser(entry.TargetID);
                markup = "<b>" + user.GetEscapedString() + "</b> " +
                         "pinned a message by <b>" +
                         target_user->GetEscapedString() +
                         "</b>";
            } break;
            case AuditLogActionType::MESSAGE_UNPIN: {
                const auto target_user = discord.GetUser(entry.TargetID);
                markup = "<b>" + user.GetEscapedString() + "</b> " +
                         "unpinned a message by <b>" +
                         target_user->GetEscapedString() +
                         "</b>";
            } break;
            default:
                markup = "<i>Unknown action</i>";
                break;
        }

        label->set_markup(markup);
        expander->set_label_widget(*label);

        if (entry.Reason.has_value()) {
            extra_markup.push_back("With reason <b>" +
                                   Glib::Markup::escape_text(*entry.Reason) +
                                   "</b>");
        }

        if (extra_markup.size() == 1)
            expander->set_expanded(true);

        auto contents = Gtk::manage(new Gtk::Box(Gtk::ORIENTATION_VERTICAL));
        for (const auto &extra : extra_markup) {
            auto extra_label = Gtk::manage(new Gtk::Label);
            extra_label->set_markup(extra);
            extra_label->set_halign(Gtk::ALIGN_START);
            extra_label->set_margin_start(25);
            extra_label->set_ellipsize(Pango::ELLIPSIZE_END);
            contents->add(*extra_label);
        }
        expander->add(*contents);
        expander->set_margin_bottom(5);
        expander->show_all();
        m_list.add(*expander);
    }
}
