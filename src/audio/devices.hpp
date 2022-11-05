#pragma once

#include <gtkmm/liststore.h>
#include <miniaudio.h>
#include <optional>

class AudioDevices {
public:
    AudioDevices();

    Glib::RefPtr<Gtk::ListStore> GetPlaybackDeviceModel();

    void SetDevices(ma_device_info *pPlayback, ma_uint32 playback_count, ma_device_info *pCapture, ma_uint32 capture_count);
    std::optional<ma_device_id> GetDeviceIDFromModel(const Gtk::TreeModel::iterator &iter);

private:
    class PlaybackColumns : public Gtk::TreeModel::ColumnRecord {
    public:
        PlaybackColumns();

        Gtk::TreeModelColumn<Glib::ustring> Name;
        Gtk::TreeModelColumn<ma_device_id> DeviceID;
    };
    PlaybackColumns m_playback_columns;
    Glib::RefPtr<Gtk::ListStore> m_playback;
};
