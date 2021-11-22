#pragma once
#include <gtkmm.h>
#include <optional>
#include "discord/objects.hpp"

class VerificationGateDialog : public Gtk::Dialog {
public:
    VerificationGateDialog(Gtk::Window &parent, Snowflake guild_id);
    const VerificationGateInfoObject &GetVerificationGate() const;

protected:
    void OnVerificationGateFetch(const std::optional<VerificationGateInfoObject> &info);

    VerificationGateInfoObject m_gate_info;

    Gtk::Label m_description;
    Gtk::ScrolledWindow m_scroll_rules;
    Gtk::ListBox m_rules;
    Gtk::ButtonBox m_bbox;

    Gtk::Button *m_ok_button;
};
