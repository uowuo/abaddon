#pragma once
#include <string>
#include <type_traits>
#include <glibmm/keyfile.h>

class SettingsManager {
public:
    struct Settings {
        // [discord]
        std::string APIBaseURL;
        std::string GatewayURL;
        std::string DiscordToken;
        bool UseMemoryDB;
        bool Prefetch;
        bool Autoconnect;
        bool UseKeychain;

        // [gui]
        std::string MainCSS;
        bool AnimatedGuildHoverOnly;
        bool ShowAnimations;
        bool ShowCustomEmojis;
        bool ShowOwnerCrown;
        bool SaveState;
        bool ShowStockEmojis;
        bool Unreads;
        bool AltMenu;
        bool HideToTray;
        bool ShowDeletedIndicator;
        double FontScale;
        bool FolderIconOnly;
        bool ClassicChangeGuildOnOpen;
        int ImageEmbedClampWidth;
        int ImageEmbedClampHeight;
        bool ClassicChannels;

        // [http]
        int CacheHTTPConcurrency;
        std::string UserAgent;

        // [style]
        std::string ChannelsExpanderColor;
        std::string NSFWChannelColor;
        std::string MentionBadgeColor;
        std::string MentionBadgeTextColor;
        std::string UnreadIndicatorColor;

        // [notifications]
        bool NotificationsEnabled;
        bool NotificationsPlaySound;

        // [voice]
        std::string VAD;
        std::string Backends;

        // [windows]
        bool HideConsole;
    };

    SettingsManager(const std::string &filename);

    void Close();
    [[nodiscard]] bool IsValid() const;
    Settings &GetSettings();

private:
    void HandleReadToken();
    void HandleWriteToken();

    void DefineSettings();
    void ReadSettings();

    // a little weird because i dont want to have to change every line where settings are used
    // why this way: i dont want to have to define a setting in multiple places and the old way was ugly
    struct SettingDefinition {
        using StringPtr = std::string Settings::*;
        using BoolPtr = bool Settings::*;
        using DoublePtr = double Settings::*;
        using IntPtr = int Settings::*;

        std::string Section;
        std::string Name;

        enum SettingType {
            TypeString,
            TypeBool,
            TypeDouble,
            TypeInt,
        } Type;

        union {
            StringPtr String;
            BoolPtr Bool;
            DoublePtr Double;
            IntPtr Int;
        } Ptr;
    };

    std::unordered_map<std::string, SettingDefinition> m_definitions;

    template<typename FieldType>
    void AddSetting(const char *section, const char *name, FieldType default_value, FieldType Settings::*ptr) {
        m_settings.*ptr = default_value;
        SettingDefinition definition;
        definition.Section = section;
        definition.Name = name;
        if constexpr (std::is_same<FieldType, std::string>::value) {
            definition.Type = SettingDefinition::TypeString;
            definition.Ptr.String = ptr;
        } else if constexpr (std::is_same<FieldType, bool>::value) {
            definition.Type = SettingDefinition::TypeBool;
            definition.Ptr.Bool = ptr;
        } else if constexpr (std::is_same<FieldType, double>::value) {
            definition.Type = SettingDefinition::TypeDouble;
            definition.Ptr.Double = ptr;
        } else if constexpr (std::is_same<FieldType, int>::value) {
            definition.Type = SettingDefinition::TypeInt;
            definition.Ptr.Int = ptr;
        }
        m_definitions[name] = definition;
    }

    bool m_ok;
    std::string m_filename;
    Glib::KeyFile m_file;
    Settings m_settings;
    Settings m_read_settings;
};
