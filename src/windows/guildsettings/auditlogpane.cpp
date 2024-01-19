#include "auditlogpane.hpp"

#include <gtkmm/expander.h>

#include "abaddon.hpp"
#include "util.hpp"

using namespace std::string_literals;

GuildSettingsAuditLogPane::GuildSettingsAuditLogPane(Snowflake id)
    : GuildID(id) {
    signal_map().connect(sigc::mem_fun(*this, &GuildSettingsAuditLogPane::OnMap));
    set_name("guild-audit-log-pane");
    set_hexpand(true);
    set_vexpand(true);

    m_list.set_selection_mode(Gtk::SELECTION_NONE);
    m_list.show();
    add(m_list);
}

void GuildSettingsAuditLogPane::OnMap() {
    if (m_requested) return;
    m_requested = true;

    auto &discord = Abaddon::Get().GetDiscordClient();
    const auto self_id = discord.GetUserData().ID;

    if (discord.HasGuildPermission(self_id, GuildID, Permission::VIEW_AUDIT_LOG))
        discord.FetchAuditLog(GuildID, sigc::mem_fun(*this, &GuildSettingsAuditLogPane::OnAuditLogFetch));
}

void GuildSettingsAuditLogPane::OnAuditLogFetch(const AuditLogData &data) {
    auto &discord = Abaddon::Get().GetDiscordClient();
    auto guild = *discord.GetGuild(GuildID);
    for (const auto &entry : data.Entries) {
        if (entry.TargetID.empty()) continue;

        auto expander = Gtk::manage(new Gtk::Expander);
        auto label = Gtk::manage(new Gtk::Label);
        label->set_ellipsize(Pango::ELLIPSIZE_END);

        Glib::ustring user_markup = "<b>Unknown User</b>";
        if (entry.UserID.has_value()) {
            if (auto user = discord.GetUser(*entry.UserID); user.has_value())
                user_markup = discord.GetUser(*entry.UserID)->GetUsernameEscapedBold();
        }

        // spaghetti moment
        Glib::ustring markup;
        std::vector<Glib::ustring> extra_markup;
        switch (entry.Type) {
            case AuditLogActionType::GUILD_UPDATE: {
                markup =
                    user_markup +
                    " made changes to <b>" +
                    Glib::Markup::escape_text(guild.Name) +
                    "</b>";

                if (entry.Changes.has_value())
                    for (const auto &change : *entry.Changes) {
                        if (change.Key == "icon_hash") {
                            extra_markup.emplace_back("Set the server icon");
                        } else if (change.Key == "name") {
                            auto new_name = change.NewValue;
                            if (new_name.has_value())
                                extra_markup.push_back("Set the server name to <b>" +
                                                       Glib::Markup::escape_text(new_name->get<std::string>()) +
                                                       "</b>");
                            else
                                extra_markup.emplace_back("Set the server name");
                        }
                    }
            } break;
            case AuditLogActionType::CHANNEL_CREATE: {
                const auto type = *entry.GetNewFromKey<ChannelType>("type");
                markup = user_markup +
                         " created a " + (type == ChannelType::GUILD_VOICE ? "voice" : "text") +
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
                            extra_markup.emplace_back((*change.NewValue ? "Marked" : "Unmarked") +
                                                      " the channel as NSFW"s);
                    }

            } break;
            case AuditLogActionType::CHANNEL_UPDATE: {
                const auto target_channel = discord.GetChannel(entry.TargetID);
                if (target_channel.has_value()) {
                    markup = user_markup +
                             " made changes to <b>#" +
                             Glib::Markup::escape_text(*target_channel->Name) +
                             "</b>";
                } else {
                    markup = user_markup +
                             " made changes to <b>&lt;#" +
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
                                extra_markup.emplace_back("Cleared the topic");
                        } else if (change.Key == "nsfw" && change.NewValue.has_value()) {
                            extra_markup.emplace_back((*change.NewValue ? "Marked" : "Unmarked") + " the channel as NSFW"s);
                        } else if (change.Key == "rate_limit_per_user" && change.NewValue.has_value()) {
                            const int secs = change.NewValue->get<int>();
                            if (secs == 0)
                                extra_markup.emplace_back("Disabled slowmode");
                            else
                                extra_markup.emplace_back("Set slowmode to <b>" +
                                                          std::to_string(secs) + " seconds</b>");
                        }
                    }
            } break;
            case AuditLogActionType::CHANNEL_DELETE: {
                markup = user_markup +
                         " removed <b>#" +
                         Glib::Markup::escape_text(*entry.GetOldFromKey<std::string>("name")) +
                         "</b>";
            } break;
            case AuditLogActionType::CHANNEL_OVERWRITE_CREATE: {
                const auto channel = discord.GetChannel(entry.TargetID);
                if (channel.has_value()) {
                    markup = user_markup +
                             " created channel overrides for <b>#" +
                             Glib::Markup::escape_text(*channel->Name) + "</b>";
                } else {
                    markup = user_markup +
                             " created channel overrides for <b>&lt;#" +
                             entry.TargetID + "&gt;</b>";
                }
            } break;
            case AuditLogActionType::CHANNEL_OVERWRITE_UPDATE: {
                const auto channel = discord.GetChannel(entry.TargetID);
                if (channel.has_value()) {
                    markup = user_markup +
                             " updated channel overrides for <b>#" +
                             Glib::Markup::escape_text(*channel->Name) + "</b>";
                } else {
                    markup = user_markup +
                             " updated channel overrides for <b>&lt;#" +
                             entry.TargetID + "&gt;</b>";
                }
            } break;
            case AuditLogActionType::CHANNEL_OVERWRITE_DELETE: {
                const auto channel = discord.GetChannel(entry.TargetID);
                if (channel.has_value()) {
                    markup = user_markup +
                             " removed channel overrides for <b>#" +
                             Glib::Markup::escape_text(*channel->Name) + "</b>";
                } else {
                    markup = user_markup +
                             " removed channel overrides for <b>&lt;#" +
                             entry.TargetID + "&gt;</b>";
                }
            } break;
            case AuditLogActionType::MEMBER_KICK: {
                const auto target_user = discord.GetUser(entry.TargetID);
                markup = user_markup +
                         " kicked <b>" +
                         target_user->GetUsernameEscaped() +
                         "</b>";
            } break;
            case AuditLogActionType::MEMBER_PRUNE: {
                markup = user_markup +
                         " pruned <b>" +
                         *entry.Options->MembersRemoved +
                         "</b> members";
                extra_markup.emplace_back("For <b>" +
                                          *entry.Options->DeleteMemberDays +
                                          " days</b> of inactivity");
            } break;
            case AuditLogActionType::MEMBER_BAN_ADD: {
                const auto target_user = discord.GetUser(entry.TargetID);
                markup = user_markup +
                         " banned <b>" +
                         target_user->GetUsernameEscaped() +
                         "</b>";
            } break;
            case AuditLogActionType::MEMBER_BAN_REMOVE: {
                const auto target_user = discord.GetUser(entry.TargetID);
                markup = user_markup +
                         " removed the ban for <b>" +
                         target_user->GetUsernameEscaped() +
                         "</b>";
            } break;
            case AuditLogActionType::MEMBER_UPDATE: {
                const auto target_user = discord.GetUser(entry.TargetID);
                markup = user_markup +
                         " updated <b>" +
                         target_user->GetUsernameEscaped() +
                         "</b>";
                if (entry.Changes.has_value())
                    for (const auto &change : *entry.Changes) {
                        if (change.Key == "deaf" && change.NewValue.has_value())
                            extra_markup.emplace_back((change.NewValue->get<bool>() ? "<b>Deafened</b>"s : "<b>Undeafened</b>"s) +
                                                      " them");
                        else if (change.Key == "mute" && change.NewValue.has_value())
                            extra_markup.emplace_back((change.NewValue->get<bool>() ? "<b>Muted</b>"s : "<b>Unmuted</b>"s) +
                                                      " them");
                        else if (change.Key == "nick" && change.NewValue.has_value())
                            extra_markup.push_back("Set their nickname to <b>" +
                                                   Glib::Markup::escape_text(change.NewValue->get<std::string>()) +
                                                   "</b>");
                    }
            } break;
            case AuditLogActionType::MEMBER_ROLE_UPDATE: {
                const auto target_user = discord.GetUser(entry.TargetID);
                markup = user_markup +
                         " updated roles for <b>" +
                         target_user->GetUsernameEscaped() + "</b>";
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
                markup = user_markup +
                         " moved <b>" +
                         *entry.Options->Count +
                         " user" +
                         (*entry.Options->Count == "1" ? ""s : "s"s) +
                         "</b> to <b>" +
                         Glib::Markup::escape_text(*channel->Name) +
                         "</b>";
            } break;
            case AuditLogActionType::MEMBER_DISCONNECT: {
                markup = user_markup +
                         " disconnected <b>" +
                         *entry.Options->Count +
                         "</b> users from voice";
            } break;
            case AuditLogActionType::BOT_ADD: {
                const auto target_user = discord.GetUser(entry.TargetID);
                markup = user_markup +
                         " added <b>" +
                         target_user->GetUsernameEscaped() +
                         "</b> to the server";
            } break;
            case AuditLogActionType::ROLE_CREATE: {
                markup = user_markup +
                         " created the role <b>" +
                         *entry.GetNewFromKey<std::string>("name") +
                         "</b>";
            } break;
            case AuditLogActionType::ROLE_UPDATE: {
                const auto role = discord.GetRole(entry.TargetID);
                markup = user_markup +
                         " updated the role <b>" +
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
                                extra_markup.emplace_back("Removed the color");
                            else
                                extra_markup.emplace_back("Set the color to <b>" +
                                                          IntToCSSColor(col) +
                                                          "</b>");
                        } else if (change.Key == "permissions") {
                            extra_markup.emplace_back("Updated the permissions");
                        } else if (change.Key == "mentionable" && change.NewValue.has_value()) {
                            extra_markup.emplace_back(change.NewValue->get<bool>() ? "Mentionable" : "Not mentionable");
                        } else if (change.Key == "hoist" && change.NewValue.has_value()) {
                            extra_markup.emplace_back(change.NewValue->get<bool>() ? "Not hoisted" : "Hoisted");
                        }
                    }
            } break;
            case AuditLogActionType::ROLE_DELETE: {
                markup = user_markup +
                         " deleted the role <b>" +
                         *entry.GetOldFromKey<std::string>("name") +
                         "</b>";
            } break;
            case AuditLogActionType::INVITE_CREATE: {
                const auto code = *entry.GetNewFromKey<std::string>("code");
                markup = user_markup +
                         " created an invite <b>" + code + "</b>";
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
                                extra_markup.emplace_back("Which has <b>unlimited</b> uses");
                            else
                                extra_markup.emplace_back("Which has <b>" + std::to_string(uses) + "</b> uses");
                        } else if (change.Key == "temporary" && change.NewValue.has_value()) {
                            extra_markup.emplace_back("With temporary <b>"s +
                                                      (change.NewValue->get<bool>() ? "on" : "off") +
                                                      "</b>");
                        } // no max_age cuz fuck time
                    }
            } break;
            case AuditLogActionType::INVITE_DELETE: {
                markup = user_markup +
                         " deleted an invite <b>" +
                         *entry.GetOldFromKey<std::string>("code") +
                         "</b>";
            } break;
            case AuditLogActionType::WEBHOOK_CREATE: {
                markup = user_markup +
                         " created the webhook <b>" +
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
                    markup = user_markup +
                             " updated the webhook <b>" +
                             Glib::Markup::escape_text(webhookptr->Name) +
                             "</b>";
                } else {
                    markup = user_markup +
                             " updated a webhook";
                }
                if (entry.Changes.has_value())
                    for (const auto &change : *entry.Changes) {
                        if (change.Key == "name" && change.NewValue.has_value()) {
                            extra_markup.push_back("Changed the name to <b>" +
                                                   Glib::Markup::escape_text(change.NewValue->get<std::string>()) +
                                                   "</b>");
                        } else if (change.Key == "avatar_hash") {
                            extra_markup.emplace_back("Changed the avatar");
                        } else if (change.Key == "channel_id" && change.NewValue.has_value()) {
                            const auto channel = discord.GetChannel(change.NewValue->get<Snowflake>());
                            if (channel.has_value()) {
                                extra_markup.push_back("Changed the channel to <b>#" +
                                                       Glib::Markup::escape_text(*channel->Name) +
                                                       "</b>");
                            } else {
                                extra_markup.emplace_back("Changed the channel");
                            }
                        }
                    }
            } break;
            case AuditLogActionType::WEBHOOK_DELETE: {
                markup = user_markup +
                         " deleted the webhook <b>" +
                         Glib::Markup::escape_text(*entry.GetOldFromKey<std::string>("name")) +
                         "</b>";
            } break;
            case AuditLogActionType::EMOJI_CREATE: {
                markup = user_markup +
                         " created the emoji <b>" +
                         Glib::Markup::escape_text(*entry.GetNewFromKey<std::string>("name")) +
                         "</b>";
            } break;
            case AuditLogActionType::EMOJI_UPDATE: {
                markup = user_markup +
                         " updated the emoji <b>" +
                         Glib::Markup::escape_text(*entry.GetOldFromKey<std::string>("name")) +
                         "</b>";
                extra_markup.push_back("Changed the name from <b>" +
                                       Glib::Markup::escape_text(*entry.GetOldFromKey<std::string>("name")) +
                                       "</b> to <b>" +
                                       Glib::Markup::escape_text(*entry.GetNewFromKey<std::string>("name")) +
                                       "</b>");
            } break;
            case AuditLogActionType::EMOJI_DELETE: {
                markup = user_markup +
                         " deleted the emoji <b>" +
                         Glib::Markup::escape_text(*entry.GetOldFromKey<std::string>("name")) +
                         "</b>";
            } break;
            case AuditLogActionType::MESSAGE_DELETE: {
                const auto channel = discord.GetChannel(*entry.Options->ChannelID);
                const auto count = *entry.Options->Count;
                if (channel.has_value()) {
                    markup = user_markup +
                             " deleted <b>" + count + "</b> messages in <b>#" +
                             Glib::Markup::escape_text(*channel->Name) +
                             "</b>";
                } else {
                    markup = user_markup +
                             " deleted <b>" + count + "</b> messages";
                }
            } break;
            case AuditLogActionType::MESSAGE_BULK_DELETE: {
                const auto channel = discord.GetChannel(entry.TargetID);
                if (channel.has_value()) {
                    markup = user_markup +
                             " deleted <b>" +
                             *entry.Options->Count +
                             "</b> messages in <b>#" +
                             Glib::Markup::escape_text(*channel->Name) +
                             "</b>";
                } else {
                    markup = user_markup +
                             " deleted <b>" +
                             *entry.Options->Count +
                             "</b> messages";
                }
            } break;
            case AuditLogActionType::MESSAGE_PIN: {
                const auto target_user = discord.GetUser(entry.TargetID);
                markup = user_markup +
                         " pinned a message by <b>" +
                         target_user->GetUsernameEscaped() +
                         "</b>";
            } break;
            case AuditLogActionType::MESSAGE_UNPIN: {
                const auto target_user = discord.GetUser(entry.TargetID);
                markup = user_markup +
                         " unpinned a message by <b>" +
                         target_user->GetUsernameEscaped() +
                         "</b>";
            } break;
            case AuditLogActionType::STAGE_INSTANCE_CREATE: {
                const auto channel = discord.GetChannel(*entry.Options->ChannelID);
                if (channel.has_value()) {
                    markup = user_markup +
                             " started the stage for <b>" +
                             Glib::Markup::escape_text(*channel->Name) +
                             "</b>";
                } else {
                    markup = user_markup +
                             " started the stage for <b>" +
                             std::to_string(*entry.Options->ChannelID) +
                             "</b>";
                }

                if (entry.Changes.has_value()) {
                    for (const auto &change : *entry.Changes) {
                        if (change.Key == "topic" && change.NewValue.has_value()) {
                            extra_markup.push_back(
                                "Set the topic to <b>" +
                                Glib::Markup::escape_text(change.NewValue->get<std::string>()) +
                                "</b>");
                        } else if (change.Key == "privacy_level" && change.NewValue.has_value()) {
                            Glib::ustring str = Glib::Markup::escape_text(GetStagePrivacyDisplayString(change.NewValue->get<StagePrivacy>()));
                            extra_markup.push_back(
                                "Set the privacy level to <b>" +
                                str +
                                "</b>");
                        }
                    }
                }
            } break;
            case AuditLogActionType::STAGE_INSTANCE_UPDATE: {
                const auto channel = discord.GetChannel(*entry.Options->ChannelID);
                if (channel.has_value()) {
                    markup = user_markup +
                             " updated the stage for <b>" +
                             Glib::Markup::escape_text(*channel->Name) +
                             "</b>";
                } else {
                    markup = user_markup +
                             " updated the stage for <b>" +
                             std::to_string(*entry.Options->ChannelID) +
                             "</b>";
                }

                if (entry.Changes.has_value()) {
                    for (const auto &change : *entry.Changes) {
                        if (change.Key == "topic" && change.NewValue.has_value()) {
                            extra_markup.push_back(
                                "Set the topic to <b>" +
                                Glib::Markup::escape_text(change.NewValue->get<std::string>()) +
                                "</b>");
                        } else if (change.Key == "privacy_level" && change.NewValue.has_value()) {
                            Glib::ustring str = Glib::Markup::escape_text(GetStagePrivacyDisplayString(change.NewValue->get<StagePrivacy>()));
                            extra_markup.push_back(
                                "Set the privacy level to <b>" +
                                str +
                                "</b>");
                        }
                    }
                }
            } break;
            case AuditLogActionType::STAGE_INSTANCE_DELETE: {
                const auto channel = discord.GetChannel(*entry.Options->ChannelID);
                if (channel.has_value()) {
                    markup = user_markup +
                             " ended the stage for <b>" +
                             Glib::Markup::escape_text(*channel->Name) +
                             "</b>";
                } else {
                    markup = user_markup +
                             " ended the stage for <b>" +
                             std::to_string(*entry.Options->ChannelID) +
                             "</b>";
                }
            } break;
            case AuditLogActionType::THREAD_CREATE: {
                const auto channel = discord.GetChannel(entry.TargetID);
                markup = user_markup +
                         " created a thread <b>" +
                         (channel.has_value()
                              ? Glib::Markup::escape_text(*channel->Name)
                              : Glib::ustring(*entry.GetNewFromKey<std::string>("name"))) +
                         "</b>";
                if (entry.Changes.has_value()) {
                    for (const auto &change : *entry.Changes) {
                        if (change.Key == "name")
                            extra_markup.push_back("Set the name to <b>" + Glib::Markup::escape_text(change.NewValue->get<std::string>()) + "</b>");
                        else if (change.Key == "archived")
                            extra_markup.emplace_back(change.NewValue->get<bool>() ? "Archived the thread" : "Unarchived the thread");
                        else if (change.Key == "auto_archive_duration")
                            extra_markup.emplace_back("Set auto archive duration to <b>"s + std::to_string(change.NewValue->get<int>()) + " minutes</b>"s);
                        else if (change.Key == "rate_limit_per_user" && change.NewValue.has_value()) {
                            const int secs = change.NewValue->get<int>();
                            if (secs == 0)
                                extra_markup.emplace_back("Disabled slowmode");
                            else
                                extra_markup.emplace_back("Set slowmode to <b>" +
                                                          std::to_string(secs) + " seconds</b>");
                        } else if (change.Key == "locked")
                            extra_markup.emplace_back(change.NewValue->get<bool>() ? "Locked the thread, restricting it to only be unarchived by moderators" : "Unlocked the thread, allowing it to be unarchived by non-moderators");
                    }
                }
            } break;
            case AuditLogActionType::THREAD_UPDATE: {
                const auto channel = discord.GetChannel(entry.TargetID);
                markup = user_markup +
                         " made changes to the thread <b>" +
                         (channel.has_value()
                              ? Glib::Markup::escape_text(*channel->Name)
                              : Glib::ustring(entry.TargetID)) +
                         "</b>";
                for (const auto &change : *entry.Changes) {
                    if (change.Key == "name")
                        extra_markup.push_back(
                            "Changed the name from <b>" +
                            Glib::Markup::escape_text(change.OldValue->get<std::string>()) +
                            "</b> to <b>" +
                            Glib::Markup::escape_text(change.NewValue->get<std::string>()) +
                            "</b>");
                    else if (change.Key == "auto_archive_duration")
                        extra_markup.emplace_back("Set auto archive duration to <b>"s + std::to_string(change.NewValue->get<int>()) + " minutes</b>"s);
                    else if (change.Key == "rate_limit_per_user" && change.NewValue.has_value()) {
                        const int secs = change.NewValue->get<int>();
                        if (secs == 0)
                            extra_markup.emplace_back("Disabled slowmode");
                        else
                            extra_markup.emplace_back("Set slowmode to <b>" +
                                                      std::to_string(secs) +
                                                      " seconds</b>");
                    } else if (change.Key == "locked")
                        extra_markup.emplace_back(change.NewValue->get<bool>() ? "Locked the thread, restricting it to only be unarchived by moderators" : "Unlocked the thread, allowing it to be unarchived by non-moderators");
                    else if (change.Key == "archived")
                        extra_markup.emplace_back(change.NewValue->get<bool>() ? "Archived the thread" : "Unarchived the thread");
                }
            } break;
            case AuditLogActionType::THREAD_DELETE: {
                markup = user_markup +
                         " deleted the thread <b>" + Glib::Markup::escape_text(*entry.GetOldFromKey<std::string>("name")) + "</b>";
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
