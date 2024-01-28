#pragma once

#include "messages/MessageElement.hpp"

#include <QString>

#include <memory>
#include <optional>

namespace chatterino {

struct Emote;
using EmotePtr = std::shared_ptr<const Emote>;

class Badge
{
public:
    Badge(QString key, QString value);

    bool operator==(const Badge &other) const;

    /**
     * The key of the badge (e.g. subscriber, moderator, chatter-cs-go-2022)
     */
    QString key;  // subscriber
                  //
    /**
     * The value of the badge (e.g. 96 for a subscriber badge, denoting that this should use the 96-month sub badge)
     */
    QString value;

    /**
     * The text of the badge
     * By default, the text is empty & will be filled in separately if text is found
     * The text is what will be displayed in the badge's tooltip
     */
    QString text;

    /**
     * The image of the badge
     * Can be nullopt if the badge just doesn't have an image, or if no image has been found set it yet
     */
    std::optional<EmotePtr> image{};

    /**
     * The badge slot this badge takes up
     */
    MessageElementFlag flag{MessageElementFlag::BadgeVanity};
};

}  // namespace chatterino

inline QDebug operator<<(QDebug debug, const chatterino::Badge &v)
{
    QDebugStateSaver saver(debug);
    debug.nospace() << "(key=" << v.key << ", value=" << v.value
                    << ", text=" << v.text << ')';

    return debug;
}
