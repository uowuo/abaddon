#include "system_audio.hpp"

namespace AbaddonClient::Audio {

SystemAudio::SystemAudio(Context &context) noexcept :
    m_engine(context) {}

std::string_view SystemAudio::GetSoundPath(SystemSound sound) noexcept {
    switch (sound) {
#ifdef ENABLE_NOTIFICATION_SOUNDS
        case SystemSound::NOTIFICATION_SOUND: {
            return "/sound/message.mp3";
        }
#endif

#ifdef WITH_VOICE
        case SystemSound::VOICE_CONNECTED: {
            return "/sound/voice_connected.mp3";
        }
        case SystemSound::VOICE_DISCONNECTED: {
            return "/sound/voice_disconnected.mp3";
        }
        case SystemSound::VOICE_MUTED: {
            return "/sound/voice_muted.mp3";
        }
        case SystemSound::VOICE_UNMUTED: {
            return "/sound/voice_unmuted.mp3";
        }
        case SystemSound::VOICE_DEAFENED: {
            return "/sound/voice_deafened.mp3";
        }
        case SystemSound::VOICE_UNDEAFENED: {
            return "/sound/voice_undeafened.mp3";
        }
#endif
    }

    return "";
}

void SystemAudio::PlaySound(SystemSound sound) noexcept {
    const auto path = Abaddon::Get().GetResPath() + GetSoundPath(sound).data();
    m_engine.PlaySound(path);
}

#ifdef WITH_VOICE

void SystemAudio::BindToVoice(DiscordClient &discord) noexcept {
    discord.signal_voice_user_connect()
        .connect(sigc::mem_fun(*this, &SystemAudio::OnVoiceUserConnect));

    discord.signal_voice_user_disconnect()
        .connect(sigc::mem_fun(*this, &SystemAudio::OnVoiceUserDisconnect));
}

void SystemAudio::OnVoiceUserConnect(Snowflake user_id, Snowflake channel_id) noexcept {
    PlaySound(SystemSound::VOICE_CONNECTED);
}

void SystemAudio::OnVoiceUserDisconnect(Snowflake user_id, Snowflake channel_id) noexcept {
    PlaySound(SystemSound::VOICE_DISCONNECTED);
}

#endif

}
