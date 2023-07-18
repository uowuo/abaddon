#pragma once
#include <vector>
#include <string>
#include <glibmm/ustring.h>
#include <giomm/file.h>
#include "discord/snowflake.hpp"

struct ChatSubmitParams {
    enum AttachmentType {
        PastedImage,
        ExtantFile,
    };

    struct Attachment {
        Glib::RefPtr<Gio::File> File;
        AttachmentType Type;
        std::string Filename;
    };

    bool Silent = false;
    Snowflake ChannelID;
    Snowflake InReplyToID;
    Snowflake EditingID;
    Glib::ustring Message;
    std::vector<Attachment> Attachments;
};
