#pragma once

#include "audio/audio_engine.hpp"

namespace AbaddonClient::Audio {

class SystemAudio : public sigc::trackable {
public:
    enum SystemSound {
#ifdef ENABLE_NOTIFICATION_SOUNDS
        NOTIFICATION_SOUND,
#endif

#ifdef WITH_VOICE
        VOICE_CONNECTED,
        VOICE_DISCONNECTED,
        VOICE_MUTED,
        VOICE_UNMUTED,
        VOICE_DEAFENED,
        VOICE_UNDEAFENED,
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
