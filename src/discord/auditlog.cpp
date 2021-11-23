#include "auditlog.hpp"

void from_json(const nlohmann::json &j, AuditLogChange &m) {
    JS_D("key", m.Key);
    JS_O("old_value", m.OldValue);
    JS_O("new_value", m.NewValue);
}

void from_json(const nlohmann::json &j, AuditLogOptions &m) {
    JS_O("delete_member_days", m.DeleteMemberDays);
    JS_O("members_removed", m.MembersRemoved);
    JS_O("channel_id", m.ChannelID);
    JS_O("message_id", m.MessageID);
    JS_O("count", m.Count);
    JS_O("id", m.ID);
    JS_O("type", m.Type);
    JS_O("role_name", m.RoleName);
}

void from_json(const nlohmann::json &j, AuditLogEntry &m) {
    JS_N("target_id", m.TargetID);
    JS_O("changes", m.Changes);
    JS_N("user_id", m.UserID);
    JS_D("id", m.ID);
    JS_D("action_type", m.Type);
    JS_O("options", m.Options);
    JS_O("reason", m.Reason);
}

void from_json(const nlohmann::json &j, AuditLogData &m) {
    JS_D("audit_log_entries", m.Entries);
    JS_D("users", m.Users);
    JS_D("webhooks", m.Webhooks);
}
