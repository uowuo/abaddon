#include "notifier.hpp"

Notifier::Notifier() {}

Notifier::~Notifier() {}

void Notifier::Notify(const Glib::ustring &id, const Glib::ustring &title, const Glib::ustring &text, const Glib::ustring &default_action, const std::string &icon_path) {}

void Notifier::Withdraw(const Glib::ustring &id) {}
