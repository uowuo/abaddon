#pragma once
#include <cstdint>
#include <glibmm/i18n.h>
#include "json.hpp"
#include "misc/bitwise.hpp"
#include "snowflake.hpp"

constexpr static uint64_t PERMISSION_MAX_BIT = 36;
enum class Permission : uint64_t {
    NONE = 0,
    CREATE_INSTANT_INVITE = (1ULL << 0),                // Allows creation of instant invites
    KICK_MEMBERS = (1ULL << 1),                         // Allows kicking members
    BAN_MEMBERS = (1ULL << 2),                          // Allows banning members
    ADMINISTRATOR = (1ULL << 3),                        // Allows all permissions and bypasses channel permission overwrites
    MANAGE_CHANNELS = (1ULL << 4),                      // Allows management and editing of channels
    MANAGE_GUILD = (1ULL << 5),                         // Allows management and editing of the guild
    ADD_REACTIONS = (1ULL << 6),                        // Allows for the addition of reactions to messages
    VIEW_AUDIT_LOG = (1ULL << 7),                       // Allows for viewing of audit logs
    PRIORITY_SPEAKER = (1ULL << 8),                     // Allows for using priority speaker in a voice channel
    STREAM = (1ULL << 9),                               // Allows the user to go live
    VIEW_CHANNEL = (1ULL << 10),                        // Allows guild members to view a channel, which includes reading messages in text channels
    SEND_MESSAGES = (1ULL << 11),                       // Allows for sending messages in a channel
    SEND_TTS_MESSAGES = (1ULL << 12),                   // Allows for sending of /tts messages
    MANAGE_MESSAGES = (1ULL << 13),                     // Allows for deletion of other users messages
    EMBED_LINKS = (1ULL << 14),                         // Links sent by users with this permission will be auto-embedded
    ATTACH_FILES = (1ULL << 15),                        // Allows for uploading images and files
    READ_MESSAGE_HISTORY = (1ULL << 16),                // Allows for reading of message history
    MENTION_EVERYONE = (1ULL << 17),                    // Allows for using the @everyone tag to notify all users in a channel, and the @here tag to notify all online users in a channel
    USE_EXTERNAL_EMOJIS = (1ULL << 18),                 // Allows the usage of custom emojis from other servers
    VIEW_GUILD_INSIGHTS = (1ULL << 19),                 // Allows for viewing guild insights
    CONNECT = (1ULL << 20),                             // Allows for joining of a voice channel
    SPEAK = (1ULL << 21),                               // Allows for speaking in a voice channel
    MUTE_MEMBERS = (1ULL << 22),                        // Allows for muting members in a voice channel
    DEAFEN_MEMBERS = (1ULL << 23),                      // Allows for deafening of members in a voice channel
    MOVE_MEMBERS = (1ULL << 24),                        // Allows for moving of members between voice channels
    USE_VAD = (1ULL << 25),                             // Allows for using voice-activity-detection in a voice channel
    CHANGE_NICKNAME = (1ULL << 26),                     // Allows for modification of own nickname
    MANAGE_NICKNAMES = (1ULL << 27),                    // Allows for modification of other users nicknames
    MANAGE_ROLES = (1ULL << 28),                        // Allows management and editing of roles
    MANAGE_WEBHOOKS = (1ULL << 29),                     // Allows management and editing of webhooks
    MANAGE_GUILD_EXPRESSIONS = (1ULL << 30),            // Allows management and editing of emojis, stickers, and soundboard sounds
    USE_APPLICATION_COMMANDS = (1ULL << 31),            // Allows members to use application commands, including slash commands and context menu commands
    REQUEST_TO_SPEAK = (1ULL << 32),                    // Allows for requesting to speak in stage channels
    MANAGE_EVENTS = (1ULL << 33),                       // Allows for creating, editing, and deleting scheduled events
    MANAGE_THREADS = (1ULL << 34),                      // Allows for deleting and archiving threads, and viewing all private threads
    CREATE_PUBLIC_THREADS = (1ULL << 35),               // Allows for creating public and announcement threads
    CREATE_PRIVATE_THREADS = (1ULL << 36),              // Allows for creating private threads
    USE_EXTERNAL_STICKERS = (1ULL << 37),               // Allows the usage of custom stickers from other servers
    SEND_MESSAGES_IN_THREADS = (1ULL << 38),            // Allows for sending messages in threads
    USE_EMBEDDED_ACTIVITIES = (1ULL << 39),             // Allows for using Activities (applications with the EMBEDDED flag) in a voice channel
    MODERATE_MEMBERS = (1ULL << 40),                    // Allows for timing out users to prevent them from sending or reacting to messages in chat and threads, and from speaking in voice and stage channels
    VIEW_CREATOR_MONETIZATION_ANALYTICS = (1ULL << 41), // Allows for viewing role subscription insights
    USE_SOUNDBOARD = (1ULL << 42),                      // Allows for using soundboard in a voice channel
    CREATE_GUILD_EXPRESSIONS = (1ULL << 43),            // undocumented. Present in client
    CREATE_EVENTS = (1ULL << 44),                       // undocumented. Present in client
    USE_EXTERNAL_SOUNDS = (1ULL << 45),                 // Allows the usage of custom soundboard sounds from other servers
    SEND_VOICE_MESSAGES = (1ULL << 46),                 // Allows sending voice messages
    USE_CLYDE_AI = (1ULL << 47),                        // undocumented
    SET_VOICE_CHANNEL_STATUS = (1ULL << 48),            // undocumented. Present in Client

    ALL = 0x1FFFFFFFFFFFFULL,
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

const char *GetPermissionString(Permission perm);

const char *GetPermissionDescription(Permission perm);
