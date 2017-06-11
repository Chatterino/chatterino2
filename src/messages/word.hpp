#pragma once

#include "fontmanager.hpp"
#include "messages/lazyloadedimage.hpp"
#include "messages/link.hpp"

#include <stdint.h>
#include <QRect>
#include <QString>

namespace chatterino {
namespace messages {

class Word
{
public:
    enum Type : uint32_t {
        None = 0,
        Misc = (1 << 0),
        Text = (1 << 1),

        TimestampNoSeconds = (1 << 2),
        TimestampWithSeconds = (1 << 3),

        TwitchEmoteImage = (1 << 4),
        TwitchEmoteText = (1 << 5),
        BttvEmoteImage = (1 << 6),
        BttvEmoteText = (1 << 7),
        BttvGifEmoteImage = (1 << 8),
        BttvGifEmoteText = (1 << 9),
        FfzEmoteImage = (1 << 10),
        FfzEmoteText = (1 << 11),
        EmoteImages = TwitchEmoteImage | BttvEmoteImage | BttvGifEmoteImage | FfzEmoteImage,

        BitsStatic = (1 << 12),
        BitsAnimated = (1 << 13),

        BadgeStaff = (1 << 14),
        BadgeAdmin = (1 << 15),
        BadgeGlobalMod = (1 << 16),
        BadgeModerator = (1 << 17),
        BadgeTurbo = (1 << 18),
        BadgeBroadcaster = (1 << 19),
        BadgePremium = (1 << 20),
        BadgeChatterino = (1 << 21),
        BadgeCheer = (1 << 22),
        Badges = BadgeStaff | BadgeAdmin | BadgeGlobalMod | BadgeModerator | BadgeTurbo |
                 BadgeBroadcaster | BadgePremium | BadgeChatterino | BadgeCheer,

        Username = (1 << 23),
        BitsAmount = (1 << 24),

        ButtonBan = (1 << 25),
        ButtonTimeout = (1 << 26),

        EmojiImage = (1 << 27),
        EmojiText = (1 << 28),

        Default = TimestampNoSeconds | Badges | Username | BitsStatic | FfzEmoteImage |
                  BttvEmoteImage | BttvGifEmoteImage | TwitchEmoteImage | BitsAmount | Text |
                  ButtonBan | ButtonTimeout
    };

    Word()
    {
    }
    explicit Word(LazyLoadedImage *_image, Type getType, const QString &copytext,
                  const QString &getTooltip, const Link &getLink = Link());
    explicit Word(const QString &_text, Type getType, const QColor &getColor,
                  const QString &copytext, const QString &getTooltip, const Link &getLink = Link());

    LazyLoadedImage &getImage() const;
    const QString &getText() const;
    int getWidth() const;
    int getHeight() const;
    void setSize(int _width, int _height);

    bool isImage() const;
    bool isText() const;
    const QString &getCopyText() const;
    bool hasTrailingSpace() const;
    QFont &getFont() const;
    QFontMetrics &getFontMetrics() const;
    Type getType() const;
    const QString &getTooltip() const;
    const QColor &getColor() const;
    const Link &getLink() const;
    int getXOffset() const;
    int getYOffset() const;
    void setOffset(int _xOffset, int _yOffset);

    std::vector<short> &getCharacterWidthCache() const;

private:
    LazyLoadedImage *_image;
    QString _text;
    QColor _color;
    bool _isImage;

    Type _type;
    QString _copyText;
    QString _tooltip;

    int _width = 16;
    int _height = 16;
    int _xOffset = 0;
    int _yOffset = 0;

    bool _hasTrailingSpace;
    FontManager::Type _font = FontManager::Medium;
    Link _link;

    mutable std::vector<short> _characterWidthCache;
};

}  // namespace messages
}  // namespace chatterino
