#pragma once
#include <vector>
#include <string>
#include <glibmm/ustring.h>
#include "discord/snowflake.hpp"

struct ChatSubmitParams {
    enum AttachmentType {
        PastedImage,
        ExtantFile,
    };

    struct Attachment {
        std::string Path;
        AttachmentType Type;
    };

    Snowflake ChannelID;
    Snowflake InReplyToID;
    Glib::ustring Message;
    std::vector<Attachment> Attachments;
};
