#include "devices.hpp"

AudioDevices::AudioDevices()
    : m_playback(Gtk::ListStore::create(m_playback_columns)) {
}

Glib::RefPtr<Gtk::ListStore> AudioDevices::GetPlaybackDeviceModel() {
    return m_playback;
}

void AudioDevices::SetDevices(ma_device_info *pPlayback, ma_uint32 playback_count, ma_device_info *pCapture, ma_uint32 capture_count) {
    m_playback->clear();

    for (ma_uint32 i = 0; i < playback_count; i++) {
        auto &d = pPlayback[i];
        auto row = *m_playback->append();
        row[m_playback_columns.Name] = d.name;
        row[m_playback_columns.DeviceID] = d.id;
    }
}

std::optional<ma_device_id> AudioDevices::GetDeviceIDFromModel(const Gtk::TreeModel::iterator &iter) {
    if (iter) {
        return static_cast<ma_device_id>((*iter)[m_playback_columns.DeviceID]);
    } else {
        return std::nullopt;
    }
}

AudioDevices::PlaybackColumns::PlaybackColumns() {
    add(Name);
    add(DeviceID);
}
