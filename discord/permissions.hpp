#pragma once
#include <cstdint>
#include "snowflake.hpp"
#include "json.hpp"
#include "../util.hpp"

constexpr static uint64_t PERMISSION_MAX_BIT = 31;
enum class Permission : uint64_t {
    NONE = 0,
    CREATE_INSTANT_INVITE = (1ULL << 0),
    KICK_MEMBERS = (1ULL << 1),
    BAN_MEMBERS = (1ULL << 2),
    ADMINISTRATOR = (1ULL << 3),
    MANAGE_CHANNELS = (1ULL << 4),
    MANAGE_GUILD = (1ULL << 5),
    ADD_REACTIONS = (1ULL << 6),
    VIEW_AUDIT_LOG = (1ULL << 7),
    PRIORITY_SPEAKER = (1ULL << 8),
    STREAM = (1ULL << 9),
    VIEW_CHANNEL = (1ULL << 10),
    SEND_MESSAGES = (1ULL << 11),
    SEND_TTS_MESSAGES = (1ULL << 12),
    MANAGE_MESSAGES = (1ULL << 13),
    EMBED_LINKS = (1ULL << 14),
    ATTACH_FILES = (1ULL << 15),
    READ_MESSAGE_HISTORY = (1ULL << 16),
    MENTION_EVERYONE = (1ULL << 17),
    USE_EXTERNAL_EMOJIS = (1ULL << 18),
    VIEW_GUILD_INSIGHTS = (1ULL << 19),
    CONNECT = (1ULL << 20),
    SPEAK = (1ULL << 21),
    MUTE_MEMBERS = (1ULL << 22),
    DEAFEN_MEMBERS = (1ULL << 23),
    MOVE_MEMBERS = (1ULL << 24),
    USE_VAD = (1ULL << 25), // voice activity detection
    CHANGE_NICKNAME = (1ULL << 26),
    MANAGE_NICKNAMES = (1ULL << 27),
    MANAGE_ROLES = (1ULL << 28),
    MANAGE_WEBHOOKS = (1ULL << 29),
    MANAGE_EMOJIS = (1ULL << 30),
    USE_APPLICATION_COMMANDS = (1ULL << 31),

    ALL = 0xFFFFFFFF,
};
template<>
struct Bitwise<Permission> {
    static const bool enable = true;
};

struct PermissionOverwrite {
    enum OverwriteType : uint8_t {
        ROLE = 0,
        MEMBER = 1,
    };

    Snowflake ID;
    OverwriteType Type;
    Permission Allow;
    Permission Deny;

    friend void from_json(const nlohmann::json &j, PermissionOverwrite &m);
};

constexpr const char *GetPermissionString(Permission perm) {
    switch (perm) {
        case Permission::NONE:
            return "None";
        case Permission::CREATE_INSTANT_INVITE:
            return "Create Invite";
        case Permission::KICK_MEMBERS:
            return "Kick Members";
        case Permission::BAN_MEMBERS:
            return "Ban Members";
        case Permission::ADMINISTRATOR:
            return "Administrator";
        case Permission::MANAGE_CHANNELS:
            return "Manage Channels";
        case Permission::MANAGE_GUILD:
            return "Manage Server";
        case Permission::ADD_REACTIONS:
            return "Add Reactions";
        case Permission::VIEW_AUDIT_LOG:
            return "View Audit Log";
        case Permission::PRIORITY_SPEAKER:
            return "Use Priority Speaker";
        case Permission::STREAM:
            return "Video";
        case Permission::VIEW_CHANNEL:
            return "View Channel";
        case Permission::SEND_MESSAGES:
            return "Send Messages";
        case Permission::SEND_TTS_MESSAGES:
            return "Use TTS";
        case Permission::MANAGE_MESSAGES:
            return "Manage Messages";
        case Permission::EMBED_LINKS:
            return "Embed Links";
        case Permission::ATTACH_FILES:
            return "Attach Files";
        case Permission::READ_MESSAGE_HISTORY:
            return "Read Message History";
        case Permission::MENTION_EVERYONE:
            return "Mention @everyone";
        case Permission::USE_EXTERNAL_EMOJIS:
            return "Use External Emojis";
        case Permission::VIEW_GUILD_INSIGHTS:
            return "View Guild Insights";
        case Permission::CONNECT:
            return "Connect to Voice";
        case Permission::SPEAK:
            return "Speak in Voice";
        case Permission::MUTE_MEMBERS:
            return "Mute Members";
        case Permission::DEAFEN_MEMBERS:
            return "Deafen Members";
        case Permission::MOVE_MEMBERS:
            return "Move Members";
        case Permission::USE_VAD:
            return "Use Voice Activation";
        case Permission::CHANGE_NICKNAME:
            return "Change Nickname";
        case Permission::MANAGE_NICKNAMES:
            return "Manage Nicknames";
        case Permission::MANAGE_ROLES:
            return "Manage Roles";
        case Permission::MANAGE_WEBHOOKS:
            return "Manage Webhooks";
        case Permission::MANAGE_EMOJIS:
            return "Manage Emojis";
        case Permission::USE_APPLICATION_COMMANDS:
            return "Use Slash Commands";
        default:
            return "Unknown Permission";
    }
}

constexpr const char* GetPermissionDescription(Permission perm) {
    switch (perm) {
        case Permission::NONE:
            return "";
        case Permission::CREATE_INSTANT_INVITE:
            return "Allows members to invite new people to this server.";
        case Permission::KICK_MEMBERS:
            return "Allows members to remove other members from this server. Kicked members will be able to rejoin if they have another invite.";
        case Permission::BAN_MEMBERS:
            return "Allows members to permanently ban other members from this server.";
        case Permission::ADMINISTRATOR:
            return "Members with this permission will have every permission and will also bypass all channel specific permissions or restrictions (for example, these members would get access to all private channels). This is a dangerous permission to grant.";
        case Permission::MANAGE_CHANNELS:
            return "Allows members to create, edit, or delete channels.";
        case Permission::MANAGE_GUILD:
            return "Allows members to change this server's name, switch regions, and add bots to this server.";
        case Permission::ADD_REACTIONS:
            return "Allows members to add new emoji reactions to a message. If this permission is disabled, members can still react using any existing reactions on a message.";
        case Permission::VIEW_AUDIT_LOG:
            return "Allows members to view a record of who made which changes in this server.";
        case Permission::PRIORITY_SPEAKER:
            return "Allows members to be more easily heard in voice channels. When activated, the volume of others without this permission will be automatically lowered. Priority Speaker is activated by using the Push to Talk (Priority) keybind.";
        case Permission::STREAM:
            return "Allows members to share their video, screen share, or stream a game in this server.";
        case Permission::VIEW_CHANNEL:
            return "Allows members to view channels by default (excluding private channels).";
        case Permission::SEND_MESSAGES:
            return "Allows members to send messages in text channels.";
        case Permission::SEND_TTS_MESSAGES:
            return "Allows members to send text-to-speech messages by starting a message with /tts. These messages can be heard by anyone focused on thsi channel.";
        case Permission::MANAGE_MESSAGES:
            return "Allows members to delete messages by other members or pin any message";
        case Permission::EMBED_LINKS:
            return "Allows links that members share to show embedded content in text channels.";
        case Permission::ATTACH_FILES:
            return "Allows members to upload files or media in text channels.";
        case Permission::READ_MESSAGE_HISTORY:
            return "Allows members to read previous messages sent in channels. If this permission is disabled, members only see messages sent when they are online and focused on that channel.";
        case Permission::MENTION_EVERYONE:
            return "Allows members to use @everyone (everyone in the server) or @here (only online members in that channel). They can also @mention all roles, even if the role's \"Allow anyone to mention this role\" permission is disabled.";
        case Permission::USE_EXTERNAL_EMOJIS:
            return "Allows members to use emoji from other servers, if they're a Discord Nitro member";
        case Permission::VIEW_GUILD_INSIGHTS:
            return "";
        case Permission::CONNECT:
            return "Allows members to join voice channels and hear others.";
        case Permission::SPEAK:
            return "Allows members to talk in voice channels. If this permission is disabled, members are default muted until somebody with the \"Mute Members\" permission un-mutes them.";
        case Permission::MUTE_MEMBERS:
            return "Allows members to mute other members in voice channels for everyone.";
        case Permission::DEAFEN_MEMBERS:
            return "Allows members to deafen other members in voice channels, which means they won't be able to speak or hear others.";
        case Permission::MOVE_MEMBERS:
            return "Allows members to move other members between voice channels that the member with the permission has access to.";
        case Permission::USE_VAD:
            return "Allows members to speak in voice channels by simply talking. If this permission is disabled, members are required to use Push-to-talk. Good for controlling background noise or noisy members.";
        case Permission::CHANGE_NICKNAME:
            return "Allows members to change their own nickname, a custom name for just this server.";
        case Permission::MANAGE_NICKNAMES:
            return "Allows members to change the nicknames of other members.";
        case Permission::MANAGE_ROLES:
            return "Allows members to create new roles and edit or delete roles lower than their highest role. Also allows members to change permissions of individual channels that they have access to.";
        case Permission::MANAGE_WEBHOOKS:
            return "Allows members to create, edit, or delete webhooks, which can post messages from other apps or sites into this server.";
        case Permission::MANAGE_EMOJIS:
            return "Allows members to add or remove custom emojis in this server.";
        case Permission::USE_APPLICATION_COMMANDS:
            return "Allows members to use slash commands in text channels.";
        default:
            return "";
    }
}
