#ifdef WITH_VOICE

// clang-format off

#include "devices.hpp"
#include <cstring>
#include <spdlog/spdlog.h>

// clang-format on

AudioDevices::AudioDevices()
    : m_playback(Gtk::ListStore::create(m_playback_columns))
    , m_capture(Gtk::ListStore::create(m_capture_columns)) {
}

Glib::RefPtr<Gtk::ListStore> AudioDevices::GetPlaybackDeviceModel() {
    return m_playback;
}

Glib::RefPtr<Gtk::ListStore> AudioDevices::GetCaptureDeviceModel() {
    return m_capture;
}

void AudioDevices::SetDevices(ma_device_info *pPlayback, ma_uint32 playback_count, ma_device_info *pCapture, ma_uint32 capture_count) {
    m_playback->clear();

    for (ma_uint32 i = 0; i < playback_count; i++) {
        auto &d = pPlayback[i];

        auto row = *m_playback->append();
        row[m_playback_columns.Name] = d.name;
        row[m_playback_columns.DeviceID] = d.id;

        if (d.isDefault) {
            m_default_playback_iter = row;
            SetActivePlaybackDevice(row);
        }
    }

    m_capture->clear();

    for (ma_uint32 i = 0; i < capture_count; i++) {
        auto &d = pCapture[i];

        auto row = *m_capture->append();
        row[m_capture_columns.Name] = d.name;
        row[m_capture_columns.DeviceID] = d.id;

        if (d.isDefault) {
            m_default_capture_iter = row;
            SetActiveCaptureDevice(row);
        }
    }

    if (!m_default_playback_iter) {
        spdlog::get("audio")->warn("No default playback device found");
    }

    if (!m_default_capture_iter) {
        spdlog::get("audio")->warn("No default capture device found");
    }
}

std::optional<ma_device_id> AudioDevices::GetPlaybackDeviceIDFromModel(const Gtk::TreeModel::iterator &iter) const {
    if (iter) {
        return static_cast<ma_device_id>((*iter)[m_playback_columns.DeviceID]);
    }

    return std::nullopt;
}

std::optional<ma_device_id> AudioDevices::GetCaptureDeviceIDFromModel(const Gtk::TreeModel::iterator &iter) const {
    if (iter) {
        return static_cast<ma_device_id>((*iter)[m_capture_columns.DeviceID]);
    }

    return std::nullopt;
}

std::optional<ma_device_id> AudioDevices::GetDefaultPlayback() const {
    if (m_default_playback_iter) {
        return static_cast<ma_device_id>((*m_default_playback_iter)[m_playback_columns.DeviceID]);
    }

    return std::nullopt;
}

std::optional<ma_device_id> AudioDevices::GetDefaultCapture() const {
    if (m_default_capture_iter) {
        return static_cast<ma_device_id>((*m_default_capture_iter)[m_capture_columns.DeviceID]);
    }

    return std::nullopt;
}

void AudioDevices::SetActivePlaybackDevice(const Gtk::TreeModel::iterator &iter) {
    m_active_playback_iter = iter;
}

void AudioDevices::SetActiveCaptureDevice(const Gtk::TreeModel::iterator &iter) {
    m_active_capture_iter = iter;
}

Gtk::TreeModel::iterator AudioDevices::GetActivePlaybackDevice() {
    return m_active_playback_iter;
}

Gtk::TreeModel::iterator AudioDevices::GetActiveCaptureDevice() {
    return m_active_capture_iter;
}

AudioDevices::PlaybackColumns::PlaybackColumns() {
    add(Name);
    add(DeviceID);
}

AudioDevices::CaptureColumns::CaptureColumns() {
    add(Name);
    add(DeviceID);
}
#endif
