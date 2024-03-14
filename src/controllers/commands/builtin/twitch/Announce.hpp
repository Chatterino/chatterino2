#pragma once

class QString;

namespace chatterino {

struct CommandContext;

}  // namespace chatterino

namespace chatterino::commands {

/// /announce
QString sendAnnouncement(const CommandContext &ctx);

/// /announceblue
QString sendAnnouncementBlue(const CommandContext &ctx);

/// /announcegreen
QString sendAnnouncementGreen(const CommandContext &ctx);

/// /announceorange
QString sendAnnouncementOrange(const CommandContext &ctx);

/// /announcepurple
QString sendAnnouncementPurple(const CommandContext &ctx);

}  // namespace chatterino::commands
