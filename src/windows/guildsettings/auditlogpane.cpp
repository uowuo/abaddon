#include "auditlogpane.hpp"

#include <glibmm/i18n.h>
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

        Glib::ustring user_markup = Glib::ustring::compose("<b>%1</b>", _("Unknown User"));
        if (entry.UserID.has_value()) {
            if (auto user = discord.GetUser(*entry.UserID); user.has_value())
                user_markup = discord.GetUser(*entry.UserID)->GetUsernameEscapedBold();
        }

        // spaghetti moment
        Glib::ustring markup;
        std::vector<Glib::ustring> extra_markup;
        switch (entry.Type) {
            case AuditLogActionType::GUILD_UPDATE: {
                markup = Glib::ustring::compose(_("%1 made changes to <b>%2</b>"),
                                                user_markup, Glib::Markup::escape_text(guild.Name));

                if (entry.Changes.has_value())
                    for (const auto &change : *entry.Changes) {
                        if (change.Key == "icon_hash") {
                            extra_markup.emplace_back(_("Set the server icon"));
                        } else if (change.Key == "name") {
                            auto new_name = change.NewValue;
                            if (new_name.has_value())
                                extra_markup.push_back(
                                    Glib::ustring::compose(_("Set the server name to <b>%1</b>"),
                                                           Glib::Markup::escape_text(new_name->get<std::string>())));
                            else
                                extra_markup.emplace_back(_("Set the server name"));
                        }
                    }
            } break;
            case AuditLogActionType::CHANNEL_CREATE: {
                const auto type = *entry.GetNewFromKey<ChannelType>("type");
                if (type == ChannelType::GUILD_VOICE) {
                    markup = Glib::ustring::compose(_("%1 created a voice channel <b>#%2</b>"),
                                                    user_markup, Glib::Markup::escape_text(*entry.GetNewFromKey<std::string>("name")));
                } else {
                    markup = Glib::ustring::compose(_("%1 created a text channel <b>#%2</b>"),
                                                    user_markup, Glib::Markup::escape_text(*entry.GetNewFromKey<std::string>("name")));
                }

                if (entry.Changes.has_value())
                    for (const auto &change : *entry.Changes) {
                        if (change.Key == "name" && change.NewValue.has_value())
                            extra_markup.push_back(
                                Glib::ustring::compose(_("Set the name to <b>%1</b>"),
                                                       Glib::Markup::escape_text(change.NewValue->get<std::string>())));
                        else if (change.Key == "nsfw" && change.NewValue.has_value())
                            if (*change.NewValue) {
                                extra_markup.emplace_back(_("Marked the channel as NSFW"));
                            } else {
                                extra_markup.emplace_back(_("Unmarked the channel as NSFW"));
                            }
                    }
            } break;
            case AuditLogActionType::CHANNEL_UPDATE: {
                const auto target_channel = discord.GetChannel(entry.TargetID);
                if (target_channel.has_value()) {
                    markup = Glib::ustring::compose(_("%1 made changes to <b>#%2</b>"),
                                                    user_markup, Glib::Markup::escape_text(*target_channel->Name));
                } else {
                    markup = Glib::ustring::compose(_("%1 made changes to <b>&lt;#%2&gt;</b>"),
                                                    user_markup, Glib::Markup::escape_text(*target_channel->Name));
                }

                if (entry.Changes.has_value())
                    for (const auto &change : *entry.Changes) {
                        if (change.Key == "name" && change.NewValue.has_value()) {
                            if (change.OldValue.has_value()) {
                                auto old_name = Glib::Markup::escape_text(change.OldValue->get<std::string>());
                                auto new_name = Glib::Markup::escape_text(change.NewValue->get<std::string>());
                                extra_markup.push_back(Glib::ustring::compose(_("Changed the name from <b>%1</b> to <b>%2</b>"),
                                                                              old_name, new_name));
                            } else {
                                auto new_name = Glib::Markup::escape_text(change.NewValue->get<std::string>());
                                extra_markup.push_back(Glib::ustring::compose(_("Changed the name to <b>%1</b>"), new_name));
                            }
                        } else if (change.Key == "topic") {
                            if (change.NewValue.has_value()) {
                                auto new_topic = Glib::Markup::escape_text(change.NewValue->get<std::string>());
                                extra_markup.push_back(Glib::ustring::compose(_("Changed the topic to <b>%1</b>"), new_topic));
                            } else {
                                extra_markup.emplace_back(_("Cleared the topic"));
                            }
                        } else if (change.Key == "nsfw" && change.NewValue.has_value()) {
                            if (*change.NewValue) {
                                extra_markup.emplace_back(_("Marked the channel as NSFW"));
                            } else {
                                extra_markup.emplace_back(_("Unmarked the channel as NSFW"));
                            }
                        } else if (change.Key == "rate_limit_per_user" && change.NewValue.has_value()) {
                            const int secs = change.NewValue->get<int>();
                            if (secs == 0)
                                extra_markup.emplace_back(_("Disabled slowmode"));
                            else
                                extra_markup.emplace_back(Glib::ustring::compose(_("Set slowmode to <b>%1 seconds</b>"), secs));
                        }
                    }
            } break;
            case AuditLogActionType::CHANNEL_DELETE: {
                auto deleted_channel_name = Glib::Markup::escape_text(*entry.GetOldFromKey<std::string>("name"));
                markup = Glib::ustring::compose(_("%1 removed <b>#%2</b>"), user_markup, deleted_channel_name);
            } break;
            case AuditLogActionType::CHANNEL_OVERWRITE_CREATE: {
                const auto channel = discord.GetChannel(entry.TargetID);
                if (channel.has_value()) {
                    auto channel_name = Glib::Markup::escape_text(*channel->Name);
                    markup = Glib::ustring::compose(_("%1 created channel overrides for <b>#%2</b>"), user_markup, channel_name);
                } else {
                    markup = Glib::ustring::compose(_("%1 created channel overrides for <b>&lt;#%2&gt;</b>"), user_markup, entry.TargetID);
                }
            } break;
            case AuditLogActionType::CHANNEL_OVERWRITE_UPDATE: {
                const auto channel = discord.GetChannel(entry.TargetID);
                if (channel.has_value()) {
                    auto channel_name = Glib::Markup::escape_text(*channel->Name);
                    markup = Glib::ustring::compose(_("%1 updated channel overrides for <b>#%2</b>"), user_markup, channel_name);
                } else {
                    markup = Glib::ustring::compose(_("%1 updated channel overrides for <b>&lt;#%2&gt;</b>"), user_markup, entry.TargetID);
                }
            } break;
            case AuditLogActionType::CHANNEL_OVERWRITE_DELETE: {
                const auto channel = discord.GetChannel(entry.TargetID);
                if (channel.has_value()) {
                    auto channel_name = Glib::Markup::escape_text(*channel->Name);
                    markup = Glib::ustring::compose(_("%1 removed channel overrides for <b>#%2</b>"), user_markup, channel_name);
                } else {
                    markup = Glib::ustring::compose(_("%1 removed channel overrides for <b>&lt;#%2&gt;</b>"), user_markup, entry.TargetID);
                }
            } break;
            case AuditLogActionType::MEMBER_KICK: {
                const auto target_user = discord.GetUser(entry.TargetID)->GetUsernameEscaped();
                markup = Glib::ustring::compose(_("%1 kicked <b>%2</b>"), user_markup, target_user);
            } break;
            case AuditLogActionType::MEMBER_PRUNE: {
                markup = Glib::ustring::compose(_("%1 pruned <b>%2</b> members"), user_markup, *entry.Options->MembersRemoved);
                extra_markup.emplace_back(Glib::ustring::compose(_("For <b>%1 days</b> of inactivity"), *entry.Options->DeleteMemberDays));
            } break;
            case AuditLogActionType::MEMBER_BAN_ADD: {
                const auto target_user = discord.GetUser(entry.TargetID)->GetUsernameEscaped();
                markup = Glib::ustring::compose(_("%1 banned <b>%2</b>"), user_markup, target_user);
            } break;
            case AuditLogActionType::MEMBER_BAN_REMOVE: {
                const auto target_user = discord.GetUser(entry.TargetID)->GetUsernameEscaped();
                markup = Glib::ustring::compose(_("%1 removed the ban for <b>%2</b>"), user_markup, target_user);
            } break;
            case AuditLogActionType::MEMBER_UPDATE: {
                const auto target_user = discord.GetUser(entry.TargetID)->GetUsername();
                markup = Glib::ustring::compose(_("%1 updated <b>%2</b>"), user_markup, target_user);

                if (entry.Changes.has_value()) {
                    for (const auto &change : *entry.Changes) {
                        if (change.Key == "deaf" && change.NewValue.has_value())
                            extra_markup.emplace_back(change.NewValue->get<bool>() ? _("<b>Deafened</b> them") : _("<b>Undeafened</b> them"));
                        else if (change.Key == "mute" && change.NewValue.has_value())
                            extra_markup.emplace_back(change.NewValue->get<bool>() ? _("<b>Muted</b> them") : _("<b>Unmuted</b> them"));
                        else if (change.Key == "nick" && change.NewValue.has_value())
                            extra_markup.push_back(Glib::ustring::compose(_("Set their nickname to <b>%1</b>"), Glib::Markup::escape_text(change.NewValue->get<std::string>())));
                    }
                }
            } break;
            case AuditLogActionType::MEMBER_ROLE_UPDATE: {
                const auto target_user = discord.GetUser(entry.TargetID)->GetUsernameEscaped();
                markup = Glib::ustring::compose(_("%1 updated roles for <b>%2</b>"), user_markup, target_user);
                if (entry.Changes.has_value()) {
                    for (const auto &change : *entry.Changes) {
                        auto role_name = Glib::Markup::escape_text(change.NewValue.value()[0].at("name").get<std::string>());
                        if (change.Key == "$remove" && change.NewValue.has_value()) {
                            extra_markup.push_back(Glib::ustring::compose(_("<b>Removed</b> a role <b>%1</b>"), role_name));
                        } else if (change.Key == "$add" && change.NewValue.has_value()) {
                            extra_markup.push_back(Glib::ustring::compose(_("<b>Added</b> a role <b>%1</b>"), role_name));
                        }
                    }
                }
            } break;

            case AuditLogActionType::MEMBER_MOVE: {
                const auto channel = discord.GetChannel(*entry.Options->ChannelID);

                if (*entry.Options->Count == "1") {
                    markup = Glib::ustring::compose(_("%1 moved <b>a user</b> to <b>%2</b>"),
                                                    user_markup, Glib::Markup::escape_text(*channel->Name));
                } else {
                    markup = Glib::ustring::compose(_("%1 moved <b>%2 users</b> to <b>%3</b>"),
                                                    user_markup, *entry.Options->Count, Glib::Markup::escape_text(*channel->Name));
                }
            } break;

            case AuditLogActionType::MEMBER_DISCONNECT: {
                markup = Glib::ustring::compose(_("%1 disconnected <b>%2 users</b> from voice"),
                                                user_markup, *entry.Options->Count);
            } break;

            case AuditLogActionType::BOT_ADD: {
                const auto target_user = discord.GetUser(entry.TargetID)->GetUsernameEscaped();
                markup = Glib::ustring::compose(_("%1 added <b>%2</b> to the server"), user_markup, target_user);
            } break;

            case AuditLogActionType::ROLE_CREATE: {
                auto role_name = *entry.GetNewFromKey<std::string>("name");
                markup = Glib::ustring::compose(_("%1 created the role <b>%2</b>"), user_markup, role_name);
            } break;

            case AuditLogActionType::ROLE_UPDATE: {
                const auto role = discord.GetRole(entry.TargetID);
                markup = Glib::ustring::compose(_("%1 updated the role <b>%2</b>"),
                                                user_markup,
                                                (role.has_value() ? Glib::Markup::escape_text(role->Name) : Glib::ustring(entry.TargetID)));
                if (entry.Changes.has_value()) {
                    for (const auto &change : *entry.Changes) {
                        if (change.Key == "name" && change.NewValue.has_value()) {
                            auto new_name = Glib::Markup::escape_text(change.NewValue->get<std::string>());
                            extra_markup.push_back(Glib::ustring::compose(_("Changed the name to <b>%1</b>"), new_name));
                        } else if (change.Key == "color" && change.NewValue.has_value()) {
                            const auto col = change.NewValue->get<int>();
                            if (col == 0) {
                                extra_markup.emplace_back(_("Removed the color"));
                            } else {
                                extra_markup.emplace_back(Glib::ustring::compose(_("Set the color to <b>%1</b>"), IntToCSSColor(col)));
                            }
                        } else if (change.Key == "permissions") {
                            extra_markup.emplace_back(_("Updated the permissions"));
                        } else if (change.Key == "mentionable" && change.NewValue.has_value()) {
                            extra_markup.emplace_back(change.NewValue->get<bool>() ? _("Mentionable") : _("Not mentionable"));
                        } else if (change.Key == "hoist" && change.NewValue.has_value()) {
                            extra_markup.emplace_back(change.NewValue->get<bool>() ? _("Not hoisted") : _("Hoisted"));
                        }
                    }
                }
            } break;

            case AuditLogActionType::ROLE_DELETE: {
                auto role_name = *entry.GetOldFromKey<std::string>("name");
                markup = Glib::ustring::compose(_("%1 deleted the role <b>%2</b>"), user_markup, role_name);
            } break;

            case AuditLogActionType::INVITE_CREATE: {
                auto code = *entry.GetNewFromKey<std::string>("code");
                markup = Glib::ustring::compose(_("%1 created an invite <b>%2</b>"), user_markup, code);
                if (entry.Changes.has_value()) {
                    for (const auto &change : *entry.Changes) {
                        if (change.Key == "channel_id" && change.NewValue.has_value()) {
                            const auto channel = discord.GetChannel(change.NewValue->get<Snowflake>());
                            if (channel.has_value()) {
                                extra_markup.push_back(Glib::ustring::compose(_("For channel <b>%1</b>"), Glib::Markup::escape_text(*channel->Name)));
                            }
                        } else if (change.Key == "max_uses" && change.NewValue.has_value()) {
                            const auto uses = change.NewValue->get<int>();
                            if (uses == 0) {
                                extra_markup.emplace_back(_("Which has <b>unlimited</b> uses"));
                            } else {
                                extra_markup.emplace_back(Glib::ustring::compose(_("Which has <b>%1</b> uses"), uses));
                            }
                        } else if (change.Key == "temporary" && change.NewValue.has_value()) {
                            if (change.NewValue->get<bool>()) {
                                extra_markup.emplace_back(_("With temporary <b>on</b>"));
                            } else {
                                extra_markup.emplace_back(_("With temporary <b>off</b>"));
                            }
                        }
                    }
                }
            } break;

            case AuditLogActionType::INVITE_DELETE: {
                markup = Glib::ustring::compose(_("%1 deleted an invite <b>%2</b>"), user_markup, *entry.GetOldFromKey<std::string>("code"));
            } break;

            case AuditLogActionType::WEBHOOK_CREATE: {
                markup = Glib::ustring::compose(
                    _("%1 created the webhook <b>%2</b>"),
                    user_markup,
                    Glib::Markup::escape_text(*entry.GetNewFromKey<std::string>("name")));
                for (const auto &change : *entry.Changes) {
                    if (change.Key == "channel_id" && change.NewValue.has_value()) {
                        const auto channel = discord.GetChannel(change.NewValue->get<Snowflake>());
                        if (channel.has_value()) {
                            extra_markup.push_back(Glib::ustring::compose(
                                _("With channel <b>#%1</b>"),
                                Glib::Markup::escape_text(*channel->Name)));
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
                    markup = Glib::ustring::compose(
                        _("%1 updated the webhook <b>%2</b>"),
                        user_markup,
                        Glib::Markup::escape_text(webhookptr->Name));
                } else {
                    markup = Glib::ustring::compose(
                        _("%1 updated a webhook"), user_markup);
                }
                if (entry.Changes.has_value()) {
                    for (const auto &change : *entry.Changes) {
                        if (change.Key == "name" && change.NewValue.has_value()) {
                            extra_markup.push_back(Glib::ustring::compose(
                                _("Changed the name to <b>%1</b>"),
                                Glib::Markup::escape_text(change.NewValue->get<std::string>())));
                        } else if (change.Key == "avatar_hash") {
                            extra_markup.emplace_back(_("Changed the avatar"));
                        } else if (change.Key == "channel_id" && change.NewValue.has_value()) {
                            const auto channel = discord.GetChannel(change.NewValue->get<Snowflake>());
                            if (channel.has_value()) {
                                extra_markup.push_back(Glib::ustring::compose(
                                    _("Changed the channel to <b>#%1</b>"),
                                    Glib::Markup::escape_text(*channel->Name)));
                            } else {
                                extra_markup.emplace_back(_("Changed the channel"));
                            }
                        }
                    }
                }
            } break;

            case AuditLogActionType::WEBHOOK_DELETE: {
                markup = Glib::ustring::compose(
                    _("%1 deleted the webhook <b>%2</b>"),
                    user_markup,
                    Glib::Markup::escape_text(*entry.GetOldFromKey<std::string>("name")));
            } break;

            case AuditLogActionType::EMOJI_CREATE: {
                markup = Glib::ustring::compose(
                    _("%1 created the emoji <b>%2</b>"),
                    user_markup,
                    Glib::Markup::escape_text(*entry.GetNewFromKey<std::string>("name")));
            } break;

            case AuditLogActionType::EMOJI_UPDATE: {
                markup = Glib::ustring::compose(
                    _("%1 updated the emoji <b>%2</b>"),
                    user_markup,
                    Glib::Markup::escape_text(*entry.GetOldFromKey<std::string>("name")));
                extra_markup.push_back(Glib::ustring::compose(
                    _("Changed the name from <b>%1</b> to <b>%2</b>"),
                    Glib::Markup::escape_text(*entry.GetOldFromKey<std::string>("name")),
                    Glib::Markup::escape_text(*entry.GetNewFromKey<std::string>("name"))));
            } break;

            case AuditLogActionType::EMOJI_DELETE: {
                markup = Glib::ustring::compose(
                    _("%1 deleted the emoji <b>%2</b>"),
                    user_markup,
                    Glib::Markup::escape_text(*entry.GetOldFromKey<std::string>("name")));
            } break;

            case AuditLogActionType::MESSAGE_BULK_DELETE:
            case AuditLogActionType::MESSAGE_DELETE: {
                const auto channel = discord.GetChannel(*entry.Options->ChannelID);
                const auto count = *entry.Options->Count;
                if (channel.has_value()) {
                    markup = Glib::ustring::compose(
                        _("%1 deleted <b>%2</b> messages in <b>#%3</b>"),
                        user_markup, count, Glib::Markup::escape_text(*channel->Name));
                } else {
                    markup = Glib::ustring::compose(
                        _("%1 deleted <b>%2</b> messages"),
                        user_markup, count);
                }
            } break;

            case AuditLogActionType::MESSAGE_PIN: {
                const auto target_user = discord.GetUser(entry.TargetID)->GetUsernameEscaped();
                markup = Glib::ustring::compose(
                    _("%1 pinned a message by <b>%2</b>"), user_markup, target_user);
            } break;

            case AuditLogActionType::MESSAGE_UNPIN: {
                const auto target_user = discord.GetUser(entry.TargetID)->GetUsernameEscaped();
                markup = Glib::ustring::compose(
                    _("%1 unpinned a message by <b>%2</b>"), user_markup, target_user);
            } break;

            case AuditLogActionType::STAGE_INSTANCE_CREATE: {
                const auto channel = discord.GetChannel(*entry.Options->ChannelID);
                markup = Glib::ustring::compose(_("%1 started the stage for <b>%2</b>"),
                                                user_markup,
                                                (channel.has_value() ? Glib::Markup::escape_text(*channel->Name) : Glib::ustring { std::to_string(*entry.Options->ChannelID) }));

                if (entry.Changes.has_value()) {
                    for (const auto &change : *entry.Changes) {
                        if (change.Key == "topic" && change.NewValue.has_value()) {
                            extra_markup.push_back(
                                Glib::ustring::compose(_("Set the topic to <b>%1</b>"),
                                                       Glib::Markup::escape_text(change.NewValue->get<std::string>())));
                        } else if (change.Key == "privacy_level" && change.NewValue.has_value()) {
                            Glib::ustring str = Glib::Markup::escape_text(GetStagePrivacyDisplayString(change.NewValue->get<StagePrivacy>()));
                            extra_markup.push_back(
                                Glib::ustring::compose(_("Set the privacy level to <b>%1</b>"), str));
                        }
                    }
                }
            } break;
            case AuditLogActionType::STAGE_INSTANCE_UPDATE: {
                const auto channel = discord.GetChannel(*entry.Options->ChannelID);
                markup = Glib::ustring::compose(_("%1 updated the stage for <b>%2</b>"),
                                                user_markup,
                                                (channel.has_value() ? Glib::Markup::escape_text(*channel->Name) : Glib::ustring { std::to_string(*entry.Options->ChannelID) }));

                if (entry.Changes.has_value()) {
                    for (const auto &change : *entry.Changes) {
                        if (change.Key == "topic" && change.NewValue.has_value()) {
                            extra_markup.push_back(
                                Glib::ustring::compose(_("Set the topic to <b>%1</b>"),
                                                       Glib::Markup::escape_text(change.NewValue->get<std::string>())));
                        } else if (change.Key == "privacy_level" && change.NewValue.has_value()) {
                            Glib::ustring str = Glib::Markup::escape_text(GetStagePrivacyDisplayString(change.NewValue->get<StagePrivacy>()));
                            extra_markup.push_back(
                                Glib::ustring::compose(_("Set the privacy level to <b>%1</b>"), str));
                        }
                    }
                }
            } break;
            case AuditLogActionType::STAGE_INSTANCE_DELETE: {
                const auto channel = discord.GetChannel(*entry.Options->ChannelID);
                markup = Glib::ustring::compose(_("%1 ended the stage for <b>%2</b>"),
                                                user_markup,
                                                (channel.has_value() ? Glib::Markup::escape_text(*channel->Name) : Glib::ustring { std::to_string(*entry.Options->ChannelID) }));
            } break;
            case AuditLogActionType::THREAD_CREATE: {
                const auto channel = discord.GetChannel(entry.TargetID);
                markup = Glib::ustring::compose(_("%1 created a thread <b>%2</b>"),
                                                user_markup.c_str(),
                                                channel.has_value() ? Glib::Markup::escape_text(*channel->Name) : Glib::ustring(*entry.GetNewFromKey<std::string>("name")));
                if (entry.Changes.has_value()) {
                    for (const auto &change : *entry.Changes) {
                        if (change.Key == "name")
                            extra_markup.push_back(Glib::ustring::compose(_("Set the name to <b>%1</b>"),
                                                                          Glib::Markup::escape_text(change.NewValue->get<std::string>())));
                        else if (change.Key == "archived")
                            extra_markup.emplace_back(change.NewValue->get<bool>() ? _("Archived the thread") : _("Unarchived the thread"));
                        else if (change.Key == "auto_archive_duration")
                            extra_markup.emplace_back(Glib::ustring::compose(_("Set auto archive duration to <b>%1 minutes</b>"), change.NewValue->get<int>()));
                        else if (change.Key == "rate_limit_per_user" && change.NewValue.has_value()) {
                            const int secs = change.NewValue->get<int>();
                            if (secs == 0)
                                extra_markup.emplace_back(_("Disabled slowmode"));
                            else
                                extra_markup.emplace_back(Glib::ustring::compose(_("Set slowmode to <b>%1 seconds</b>"), secs));
                        } else if (change.Key == "locked")
                            extra_markup.emplace_back(change.NewValue->get<bool>() ? _("Locked the thread, restricting it to only be unarchived by moderators") : _("Unlocked the thread, allowing it to be unarchived by non-moderators"));
                    }
                }
            } break;
            case AuditLogActionType::THREAD_UPDATE: {
                const auto channel = discord.GetChannel(entry.TargetID);
                markup = Glib::ustring::compose(_("%1 made changes to the thread <b>%2</b>"),
                                                user_markup,
                                                (channel.has_value()
                                                     ? Glib::Markup::escape_text(*channel->Name)
                                                     : Glib::ustring(entry.TargetID)));
                for (const auto &change : *entry.Changes) {
                    if (change.Key == "name")
                        extra_markup.push_back(Glib::ustring::compose(_("Changed the name from <b>%1</b> to <b>%2</b>"),
                                                                      Glib::Markup::escape_text(change.OldValue->get<std::string>()),
                                                                      Glib::Markup::escape_text(change.NewValue->get<std::string>())));
                    else if (change.Key == "auto_archive_duration")
                        extra_markup.emplace_back(Glib::ustring::compose(_("Set auto archive duration to <b>%1 minutes</b>"), change.NewValue->get<int>()));
                    else if (change.Key == "rate_limit_per_user" && change.NewValue.has_value()) {
                        const int secs = change.NewValue->get<int>();
                        if (secs == 0)
                            extra_markup.emplace_back(_("Disabled slowmode"));
                        else
                            extra_markup.emplace_back(Glib::ustring::compose(_("Set slowmode to <b>%1 seconds</b>"), secs));
                    } else if (change.Key == "locked")
                        extra_markup.emplace_back(change.NewValue->get<bool>() ? _("Locked the thread, restricting it to only be unarchived by moderators") : _("Unlocked the thread, allowing it to be unarchived by non-moderators"));
                    else if (change.Key == "archived")
                        extra_markup.emplace_back(change.NewValue->get<bool>() ? _("Archived the thread") : _("Unarchived the thread"));
                }
            } break;
            case AuditLogActionType::THREAD_DELETE: {
                markup = Glib::ustring::compose(_("%1 deleted the thread <b>%2</b>"),
                                                user_markup, Glib::Markup::escape_text(*entry.GetOldFromKey<std::string>("name")));
            } break;
            default:
                markup = Glib::ustring::compose("<i>%1</i>", _("Unknown action"));
                break;
        }

        label->set_markup(markup);
        expander->set_label_widget(*label);

        if (entry.Reason.has_value()) {
            extra_markup.push_back(Glib::ustring::compose(_("Reason: <b>%1</b>"),
                                                          Glib::Markup::escape_text(*entry.Reason)));
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
