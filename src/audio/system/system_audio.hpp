#pragma once

#include "audio/audio_engine.hpp"

namespace AbaddonClient::Audio {

class SystemAudio : public sigc::trackable {
public:
    enum SystemSound {
#ifdef ENABLE_NOTIFICATION_SOUNDS
        Notification,
#endif

#ifdef WITH_VOICE
        VoiceConnected,
        VoiceDisconnected,
        VoiceMuted,
        VoiceUnmuted,
        VoiceDeafened,
        VoiceUndeafened,
#endif
    };

    SystemAudio(Context &context) noexcept;

#ifdef WITH_VOICE
    void BindToVoice(DiscordClient &discord) noexcept;
#endif

    std::string_view GetSoundPath(SystemSound sound) noexcept;
    void PlaySound(SystemSound sound) noexcept;

private:

#ifdef WITH_VOICE
    void OnVoiceUserConnect(Snowflake user_id, Snowflake channel_id) noexcept;
    void OnVoiceUserDisconnect(Snowflake user_id, Snowflake channel_id) noexcept;
#endif

    AudioEngine m_engine;
};

}
