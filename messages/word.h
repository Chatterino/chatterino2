#ifndef WORD_H
#define WORD_H

#include "fonts.h"
#include "messages/lazyloadedimage.h"
#include "messages/link.h"

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
        EmoteImages = TwitchEmoteImage | BttvEmoteImage | BttvGifEmoteImage |
                      FfzEmoteImage,

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
        Badges = BadgeStaff | BadgeAdmin | BadgeGlobalMod | BadgeModerator |
                 BadgeTurbo | BadgeBroadcaster | BadgePremium |
                 BadgeChatterino | BadgeCheer,

        Username = (1 << 23),
        BitsAmount = (1 << 24),

        ButtonBan = (1 << 25),
        ButtonTimeout = (1 << 26),

        EmojiImage = (1 << 27),
        EmojiText = (1 << 28),

        Default = TimestampNoSeconds | Badges | Username | BitsStatic |
                  FfzEmoteImage | BttvEmoteImage | BttvGifEmoteImage |
                  TwitchEmoteImage | BitsAmount | Text | ButtonBan |
                  ButtonTimeout
    };

    explicit Word(LazyLoadedImage *image, Type getType, const QString &copytext,
                  const QString &getTooltip, const Link &getLink = Link());
    explicit Word(const QString &text, Type getType, const QColor &getColor,
                  const QString &copytext, const QString &getTooltip,
                  const Link &getLink = Link());

    LazyLoadedImage &
    getImage() const
    {
        return *image;
    }

    const QString &
    getText() const
    {
        return this->text;
    }

    int
    getWidth() const
    {
        return this->width;
    }

    int
    getHeight() const
    {
        return this->height;
    }

    void
    setSize(int width, int height)
    {
        this->width = width;
        this->height = height;
    }

    bool
    isImage() const
    {
        return this->_isImage;
    }

    bool
    isText() const
    {
        return !this->_isImage;
    }

    const QString &
    getCopyText() const
    {
        return this->copyText;
    }

    bool
    hasTrailingSpace() const
    {
        return this->_hasTrailingSpace;
    }

    QFont &
    getFont() const
    {
        return Fonts::getFont(this->font);
    }

    QFontMetrics &
    getFontMetrics() const
    {
        return Fonts::getFontMetrics(this->font);
    }

    Type
    getType() const
    {
        return this->type;
    }

    const QString &
    getTooltip() const
    {
        return this->tooltip;
    }

    const QColor &
    getColor() const
    {
        return this->color;
    }

    const Link &
    getLink() const
    {
        return this->link;
    }

    int
    getXOffset() const
    {
        return this->xOffset;
    }

    int
    getYOffset() const
    {
        return this->yOffset;
    }

    void
    setOffset(int xOffset, int yOffset)
    {
        this->xOffset = std::max(0, xOffset);
        this->yOffset = std::max(0, yOffset);
    }

    std::vector<short> &
    getCharacterWidthCache()
    {
        return this->characterWidthCache;
    }

private:
    LazyLoadedImage *image;
    QString text;
    QColor color;
    bool _isImage;

    Type type;
    QString copyText;
    QString tooltip;

    int width = 16;
    int height = 16;
    int xOffset = 0;
    int yOffset = 0;

    bool _hasTrailingSpace;
    Fonts::Type font = Fonts::Medium;
    Link link;

    std::vector<short> characterWidthCache;
};

}  // namespace messages
}  // namespace chatterino

#endif  // WORD_H
