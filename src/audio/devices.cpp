#ifdef WITH_VOICE

// clang-format off

#include "devices.hpp"

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
    }

    m_capture->clear();

    for (ma_uint32 i = 0; i < capture_count; i++) {
        auto &d = pCapture[i];
        auto row = *m_capture->append();
        row[m_capture_columns.Name] = d.name;
        row[m_capture_columns.DeviceID] = d.id;
    }
}

std::optional<ma_device_id> AudioDevices::GetPlaybackDeviceIDFromModel(const Gtk::TreeModel::iterator &iter) const {
    if (iter) {
        return static_cast<ma_device_id>((*iter)[m_playback_columns.DeviceID]);
    } else {
        return std::nullopt;
    }
}

std::optional<ma_device_id> AudioDevices::GetCaptureDeviceIDFromModel(const Gtk::TreeModel::iterator &iter) const {
    if (iter) {
        return static_cast<ma_device_id>((*iter)[m_capture_columns.DeviceID]);
    } else {
        return std::nullopt;
    }
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
