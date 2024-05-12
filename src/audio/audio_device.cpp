#include "audio_device.hpp"

namespace AbaddonClient::Audio {

AudioDevice::AudioDevice(Context &context, ma_device_config &&config, std::optional<ma_device_id> &&device_id) noexcept :
    m_context(context),
    m_config(std::move(config)),
    m_device_id(std::move(device_id))
{
    SyncDeviceID();
}

bool AudioDevice::Start() noexcept {
    if (m_started) {
        return true;
    }

    m_device = Miniaudio::MaDevice::Create(m_context.GetRaw(), m_config);
    if (!m_device) {
        return false;
    }

    m_started = m_device->Start();
    if (!m_started) {
        m_device.reset();
    }

    return m_started;
}

bool AudioDevice::Stop() noexcept {
    if (!m_started) {
        return true;
    }

    m_started = !m_device->Stop();

    // If we're still running something went wrong
    if (m_started) {
        return false;
    }

    m_device.reset();
    return true;
}

bool AudioDevice::ChangeDevice(ma_device_id &&device_id) noexcept {
    m_device_id = std::move(device_id);

    return RefreshDevice();
}

void AudioDevice::SyncDeviceID() noexcept {
    if (!m_device_id) {
        return;
    }

    auto& device_id = *m_device_id;

    switch (m_config.deviceType) {
        case ma_device_type_playback: {
            m_config.playback.pDeviceID = &device_id;
        } break;

        case ma_device_type_capture: {
            m_config.capture.pDeviceID = &device_id;
        } break;

        case ma_device_type_duplex: {
            m_config.playback.pDeviceID = &device_id;
            m_config.capture.pDeviceID = &device_id;
        }

        case ma_device_type_loopback: {
            m_config.capture.pDeviceID = &device_id;
        }
    }
}

bool AudioDevice::RefreshDevice() noexcept {
    m_device.reset();
    if (m_started) {
        return Start();
    }

    return true;
}

}
