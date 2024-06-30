#include "permissions.hpp"

void from_json(const nlohmann::json &j, PermissionOverwrite &m) {
    JS_D("id", m.ID);
    std::string tmp;
    m.Type = j.at("type").get<int>() == 0 ? PermissionOverwrite::ROLE : PermissionOverwrite::MEMBER;
    JS_D("allow", tmp);
    m.Allow = static_cast<Permission>(std::stoull(tmp));
    JS_D("deny", tmp);
    m.Deny = static_cast<Permission>(std::stoull(tmp));
}

const char *GetPermissionString(Permission perm) {
    switch (perm) {
        case Permission::NONE:
            return _("None");
        case Permission::CREATE_INSTANT_INVITE:
            return _("Create Invite");
        case Permission::KICK_MEMBERS:
            return _("Kick Members");
        case Permission::BAN_MEMBERS:
            return _("Ban Members");
        case Permission::ADMINISTRATOR:
            return _("Administrator");
        case Permission::MANAGE_CHANNELS:
            return _("Manage Channels");
        case Permission::MANAGE_GUILD:
            return _("Manage Server");
        case Permission::ADD_REACTIONS:
            return _("Add Reactions");
        case Permission::VIEW_AUDIT_LOG:
            return _("View Audit Log");
        case Permission::PRIORITY_SPEAKER:
            return _("Use Priority Speaker");
        case Permission::STREAM:
            return _("Video");
        case Permission::VIEW_CHANNEL:
            return _("View Channels");
        case Permission::SEND_MESSAGES:
            return _("Send Messages");
        case Permission::SEND_TTS_MESSAGES:
            return _("Use TTS");
        case Permission::MANAGE_MESSAGES:
            return _("Manage Messages");
        case Permission::EMBED_LINKS:
            return _("Embed Links");
        case Permission::ATTACH_FILES:
            return _("Attach Files");
        case Permission::READ_MESSAGE_HISTORY:
            return _("Read Message History");
        case Permission::MENTION_EVERYONE:
            return _("Mention @everyone");
        case Permission::USE_EXTERNAL_EMOJIS:
            return _("Use External Emojis");
        case Permission::VIEW_GUILD_INSIGHTS:
            return _("View Server Insights");
        case Permission::CONNECT:
            return _("Connect to Voice");
        case Permission::SPEAK:
            return _("Speak in Voice");
        case Permission::MUTE_MEMBERS:
            return _("Mute Members");
        case Permission::DEAFEN_MEMBERS:
            return _("Deafen Members");
        case Permission::MOVE_MEMBERS:
            return _("Move Members");
        case Permission::USE_VAD:
            return _("Use Voice Activation");
        case Permission::CHANGE_NICKNAME:
            return _("Change Nickname");
        case Permission::MANAGE_NICKNAMES:
            return _("Manage Nicknames");
        case Permission::MANAGE_ROLES:
            return _("Manage Roles");
        case Permission::MANAGE_WEBHOOKS:
            return _("Manage Webhooks");
        case Permission::MANAGE_GUILD_EXPRESSIONS:
            return _("Manage Expressions");
        case Permission::USE_APPLICATION_COMMANDS:
            return _("Use Application Commands");
        case Permission::MANAGE_EVENTS:
            return _("Manage Events");
        case Permission::MANAGE_THREADS:
            return _("Manage Threads");
        case Permission::CREATE_PUBLIC_THREADS:
            return _("Create Public Threads");
        case Permission::CREATE_PRIVATE_THREADS:
            return _("Create Private Threads");
        case Permission::USE_EXTERNAL_STICKERS:
            return _("Use External Stickers");
        case Permission::SEND_MESSAGES_IN_THREADS:
            return _("Send Messages In Threads");
        case Permission::USE_EMBEDDED_ACTIVITIES:
            return _("Use Activities");
        case Permission::MODERATE_MEMBERS:
            return _("Timeout Members");
        // case Permission::VIEW_CREATOR_MONETIZATION_ANALYTICS:
        //     return "";
        case Permission::USE_SOUNDBOARD:
            return _("Use Soundboard");
        case Permission::CREATE_GUILD_EXPRESSIONS:
            return _("Create Expressions");
        case Permission::CREATE_EVENTS:
            return _("Create Events");
        case Permission::USE_EXTERNAL_SOUNDS:
            return _("Use External Sounds");
        case Permission::SEND_VOICE_MESSAGES:
            return _("Send Voice Messages");
        // case Permission::USE_CLYDE_AI:
        //     return "";
        case Permission::SET_VOICE_CHANNEL_STATUS:
            return _("Set Voice Channel Status");
        default:
            return _("Unknown Permission");
    }
}

const char *GetPermissionDescription(Permission perm) {
    switch (perm) {
        case Permission::NONE:
            return "";
        case Permission::CREATE_INSTANT_INVITE:
            return _("Allows members to invite new people to this server.");
        case Permission::KICK_MEMBERS:
            return _("Allows members to remove other members from this server. Kicked members will be able to rejoin if they have another invite.");
        case Permission::BAN_MEMBERS:
            return _("Allows members to permanently ban other members from this server.");
        case Permission::ADMINISTRATOR:
            return _("Members with this permission will have every permission and will also bypass all channel specific permissions or restrictions (for example, these members would get access to all private channels). This is a dangerous permission to grant.");
        case Permission::MANAGE_CHANNELS:
            return _("Allows members to create, edit, or delete channels.");
        case Permission::MANAGE_GUILD:
            return _("Allows members to change this server's name, switch regions, and add bots to this server.");
        case Permission::ADD_REACTIONS:
            return _("Allows members to add new emoji reactions to a message. If this permission is disabled, members can still react using any existing reactions on a message.");
        case Permission::VIEW_AUDIT_LOG:
            return _("Allows members to view a record of who made which changes in this server.");
        case Permission::PRIORITY_SPEAKER:
            return _("Allows members to be more easily heard in voice channels. When activated, the volume of others without this permission will be automatically lowered. Priority Speaker is activated by using the Push to Talk (Priority) keybind.");
        case Permission::STREAM:
            return _("Allows members to share their video, screen share, or stream a game in this server.");
        case Permission::VIEW_CHANNEL:
            return _("Allows members to view channels by default (excluding private channels).");
        case Permission::SEND_MESSAGES:
            return _("Allows members to send messages in text channels.");
        case Permission::SEND_TTS_MESSAGES:
            return _("Allows members to send text-to-speech messages by starting a message with /tts. These messages can be heard by anyone focused on this channel.");
        case Permission::MANAGE_MESSAGES:
            return _("Allows members to delete messages by other members or pin any message.");
        case Permission::EMBED_LINKS:
            return _("Allows links that members share to show embedded content in text channels.");
        case Permission::ATTACH_FILES:
            return _("Allows members to upload files or media in text channels.");
        case Permission::READ_MESSAGE_HISTORY:
            return _("Allows members to read previous messages sent in channels. If this permission is disabled, members only see messages sent when they are online and focused on that channel.");
        case Permission::MENTION_EVERYONE:
            return _("Allows members to use @everyone (everyone in the server) or @here (only online members in that channel). They can also @mention all roles, even if the role's \"Allow anyone to mention this role\" permission is disabled.");
        case Permission::USE_EXTERNAL_EMOJIS:
            return _("Allows members to use emoji from other servers, if they're a Discord Nitro member.");
        case Permission::VIEW_GUILD_INSIGHTS:
            return _("Allows members to view Server Insights, which shows data on community growth, engagement, and more.");
        case Permission::CONNECT:
            return _("Allows members to join voice channels and hear others.");
        case Permission::SPEAK:
            return _("Allows members to talk in voice channels. If this permission is disabled, members are default muted until somebody with the \"Mute Members\" permission unmutes them.");
        case Permission::MUTE_MEMBERS:
            return _("Allows members to mute other members in voice channels for everyone.");
        case Permission::DEAFEN_MEMBERS:
            return _("Allows members to deafen other members in voice channels, which means they won't be able to speak or hear others.");
        case Permission::MOVE_MEMBERS:
            return _("Allows members to move other members between voice channels that the member with the permission has access to.");
        case Permission::USE_VAD:
            return _("Allows members to speak in voice channels by simply talking. If this permission is disabled, members are required to use Push-to-talk. Good for controlling background noise or noisy members.");
        case Permission::CHANGE_NICKNAME:
            return _("Allows members to change their own nickname, a custom name for just this server.");
        case Permission::MANAGE_NICKNAMES:
            return _("Allows members to change the nicknames of other members.");
        case Permission::MANAGE_ROLES:
            return _("Allows members to create new roles and edit or delete roles lower than their highest role. Also allows members to change permissions of individual channels that they have access to.");
        case Permission::MANAGE_WEBHOOKS:
            return _("Allows members to create, edit, or delete webhooks, which can post messages from other apps or sites into this server.");
        case Permission::MANAGE_GUILD_EXPRESSIONS:
            return _("Allows members to add or remove custom emoji, stickers, and sounds in this server.");
        case Permission::USE_APPLICATION_COMMANDS:
            return _("Allows members to use commands from applications, including slash commands and context menu commands.");
        case Permission::MANAGE_EVENTS:
            return _("Allows members to edit and cancel events.");
        case Permission::MANAGE_THREADS:
            return _("Allows members to rename, delete, close, and turn on slow mode for threads. They can also view private threads.");
        case Permission::CREATE_PUBLIC_THREADS:
            return _("Allows members to create threads that everyone in a channel can view.");
        case Permission::CREATE_PRIVATE_THREADS:
            return _("Allows members to create invite-only threads.");
        case Permission::USE_EXTERNAL_STICKERS:
            return _("Allows members to use stickers from other servers, if they're a Discord Nitro member.");
        case Permission::SEND_MESSAGES_IN_THREADS:
            return _("Allows members to send messages in threads.");
        case Permission::USE_EMBEDDED_ACTIVITIES:
            return _("Allows members to use Activities in this server.");
        case Permission::MODERATE_MEMBERS:
            return _("When you put a user in timeout they will not be able to send messages in chat, reply within threads, react to messages, or speak in voice or Stage channels.");
        // case Permission::VIEW_CREATOR_MONETIZATION_ANALYTICS:
        //     return "";
        case Permission::USE_SOUNDBOARD:
            return _("Allows members to send sounds from server soundboard.");
        case Permission::CREATE_GUILD_EXPRESSIONS:
            return _("Allows members to add custom emoji, stickers, and sounds in this server.");
        case Permission::CREATE_EVENTS:
            return _("Allows members to create events.");
        case Permission::USE_EXTERNAL_SOUNDS:
            return _("Allows members to use sounds from other servers, if they're a Discord Nitro member.");
        case Permission::SEND_VOICE_MESSAGES:
            return _("Allows members to send voice messages.");
        // case Permission::USE_CLYDE_AI:
        //     return "";
        case Permission::SET_VOICE_CHANNEL_STATUS:
            return _("Allows members to create and edit voice channel status.");
        default:
            return "";
    }
}