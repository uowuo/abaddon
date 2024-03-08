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

Glib::RefPtr<Gtk::ListStore> AudioDevices::GetPlaybackDeviceModel() const {
    return m_playback;
}

Glib::RefPtr<Gtk::ListStore> AudioDevices::GetCaptureDeviceModel() const {
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
            SetActivePlaybackDeviceIter(row);
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
            SetActiveCaptureDeviceIter(row);
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
    return GetPlaybackDeviceIDFromModel(m_default_playback_iter);
}

std::optional<ma_device_id> AudioDevices::GetDefaultCapture() const {
    return GetCaptureDeviceIDFromModel(m_default_capture_iter);
}

std::optional<ma_device_id> AudioDevices::GetActivePlayback() const {
    return GetPlaybackDeviceIDFromModel(m_active_playback_iter);
}

std::optional<ma_device_id> AudioDevices::GetActiveCapture() const {
    return GetCaptureDeviceIDFromModel(m_active_capture_iter);
}

void AudioDevices::SetActivePlaybackDeviceIter(const Gtk::TreeModel::iterator &iter) {
    m_active_playback_iter = iter;
}

void AudioDevices::SetActiveCaptureDeviceIter(const Gtk::TreeModel::iterator &iter) {
    m_active_capture_iter = iter;
}

Gtk::TreeModel::iterator AudioDevices::GetActivePlaybackDeviceIter() const {
    return m_active_playback_iter;
}

Gtk::TreeModel::iterator AudioDevices::GetActiveCaptureDeviceIter() const {
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
