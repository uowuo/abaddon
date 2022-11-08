#pragma once
#ifdef WITH_VOICE

// clang-format off

#include <gtkmm/liststore.h>
#include <miniaudio.h>
#include <optional>

// clang-format on

class AudioDevices {
public:
    AudioDevices();

    Glib::RefPtr<Gtk::ListStore> GetPlaybackDeviceModel();
    Glib::RefPtr<Gtk::ListStore> GetCaptureDeviceModel();

    void SetDevices(ma_device_info *pPlayback, ma_uint32 playback_count, ma_device_info *pCapture, ma_uint32 capture_count);
    std::optional<ma_device_id> GetPlaybackDeviceIDFromModel(const Gtk::TreeModel::iterator &iter) const;
    std::optional<ma_device_id> GetCaptureDeviceIDFromModel(const Gtk::TreeModel::iterator &iter) const;

private:
    class PlaybackColumns : public Gtk::TreeModel::ColumnRecord {
    public:
        PlaybackColumns();

        Gtk::TreeModelColumn<Glib::ustring> Name;
        Gtk::TreeModelColumn<ma_device_id> DeviceID;
    };
    PlaybackColumns m_playback_columns;
    Glib::RefPtr<Gtk::ListStore> m_playback;

    class CaptureColumns : public Gtk::TreeModel::ColumnRecord {
    public:
        CaptureColumns();

        Gtk::TreeModelColumn<Glib::ustring> Name;
        Gtk::TreeModelColumn<ma_device_id> DeviceID;
    };
    CaptureColumns m_capture_columns;
    Glib::RefPtr<Gtk::ListStore> m_capture;
};
#endif
