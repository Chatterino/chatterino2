// SPDX-FileCopyrightText: 2026 Contributors to Chatterino <https://chatterino.com>
//
// SPDX-License-Identifier: MIT

#include "controllers/highlights/types/AnnouncementsHighlight.hpp"

#include "controllers/highlights/HighlightCheck.hpp"
#include "controllers/highlights/HighlightResult.hpp"
#include "messages/Message.hpp"

namespace chatterino::highlights {

namespace {

const auto BLUE = std::make_shared<QColor>(QColor(102, 148, 255, 100));
const auto GREEN = std::make_shared<QColor>(QColor(96, 255, 96, 100));
const auto ORANGE = std::make_shared<QColor>(QColor(233, 210, 0, 100));
const auto PURPLE = std::make_shared<QColor>(QColor(255, 102, 237, 100));

}  // namespace

HighlightCheck AnnouncementsHighlight::buildCheck() const
{
    using H = std::remove_pointer_t<decltype(this)>;
    using Params = HighlightCheck::Params;

    return {
        [highlight = *this](const Params &p) -> std::optional<HighlightResult> {
            if (!p.messageFlags.has(MessageFlag::Announcement))
            {
                return std::nullopt;
            }

            std::shared_ptr<QColor> backgroundColor;

            if (highlight.overrideColoredAnnouncements.value_or(
                    H::OVERRIDE_COLORED_ANNOUNCEMENTS_DEFAULT))
            {
                backgroundColor = highlight.outcome.getBackgroundColor();
            }
            else
            {
                switch (p.runContext.message.announcementColor)
                {
                    case HelixAnnouncementColor::Blue:
                        backgroundColor = BLUE;
                        break;

                    case HelixAnnouncementColor::Green:
                        backgroundColor = GREEN;
                        break;

                    case HelixAnnouncementColor::Orange:
                        backgroundColor = ORANGE;
                        break;

                    case HelixAnnouncementColor::Purple:
                        backgroundColor = PURPLE;
                        break;

                    case HelixAnnouncementColor::Primary:
                    default:
                        backgroundColor =
                            highlight.outcome.getBackgroundColor();
                        break;
                }
            }

            return HighlightResult{
                .ids = {H::ID.toString()},
                .alert = highlight.outcome.alert.value_or(H::ALERT_DEFAULT),
                .sound = highlight.outcome.soundURL,
                .color = backgroundColor,
                .showInMentions = highlight.outcome.showInMentions.value_or(
                    H::SHOW_IN_MENTIONS_DEFAULT),
            };
        },
    };
}

QDebug operator<<(QDebug dbg, const AnnouncementsHighlight &v)
{
    dbg.nospace() << "AnnouncementsHighlight("
                  << "enabled:" << v.enabled << ','
                  << "overrideColoredAnnouncements:"
                  << v.overrideColoredAnnouncements << ','
                  << "outcome:" << v.outcome << ')';

    return dbg;
}

}  // namespace chatterino::highlights
