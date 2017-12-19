#pragma once

#include "fontmanager.hpp"
#include "messages/lazyloadedimage.hpp"
#include "messages/link.hpp"
#include "messages/messagecolor.hpp"

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

        // Slot 1: Twitch
        // - Staff badge
        // - Admin badge
        // - Global Moderator badge
        BadgeGlobalAuthority = (1 << 14),

        // Slot 2: Twitch
        // - Moderator badge
        // - Broadcaster badge
        BadgeChannelAuthority = (1 << 15),

        // Slot 3: Twitch
        // - Subscription badges
        BadgeSubscription = (1 << 16),

        // Slot 4: Twitch
        // - Turbo badge
        // - Prime badge
        // - Bit badges
        // - Game badges
        BadgeVanity = (1 << 17),

        // Slot 5: Chatterino
        // - Chatterino developer badge
        // - Chatterino donator badge
        // - Chatterino top donator badge
        BadgeChatterino = (1 << 18),

        // Rest of slots: ffz custom badge? bttv custom badge? mywaifu (puke) custom badge?

        Badges = BadgeGlobalAuthority | BadgeChannelAuthority | BadgeSubscription | BadgeVanity |
                 BadgeChatterino,

        Username = (1 << 19),
        BitsAmount = (1 << 20),

        ButtonBan = (1 << 21),
        ButtonTimeout = (1 << 22),

        EmojiImage = (1 << 23),
        EmojiText = (1 << 24),

        AlwaysShow = (1 << 25),

        // used in the ChannelView class to make the collapse buttons visible if needed
        Collapsed = (1 << 26),

        Default = TimestampNoSeconds | Badges | Username | BitsStatic | FfzEmoteImage |
                  BttvEmoteImage | BttvGifEmoteImage | TwitchEmoteImage | BitsAmount | Text |
                  ButtonBan | ButtonTimeout | AlwaysShow
    };

    Word()
    {
    }

    explicit Word(LazyLoadedImage *_image, Type getType, const QString &copytext,
                  const QString &tooltip, const Link &getLink = Link());
    explicit Word(const QString &_text, Type getType, const MessageColor &getColor,
                  const QString &copytext, const QString &tooltip, const Link &getLink = Link());

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
    const MessageColor &getColor() const;
    const Link &getLink() const;
    int getXOffset() const;
    int getYOffset() const;
    void setOffset(int _xOffset, int _yOffset);
    int getCharacterLength() const;

    short getCharWidth(int index) const;

private:
    LazyLoadedImage *image;
    QString text;
    MessageColor color;
    bool _isImage;

    Type type;
    QString copyText;
    QString tooltip;

    int width = 16;
    int height = 16;
    int xOffset = 0;
    int yOffset = 0;

    bool _hasTrailingSpace = true;
    FontManager::Type font = FontManager::Medium;
    Link link;

    std::vector<short> &getCharacterWidthCache() const;
    mutable std::vector<short> charWidthCache;
};

}  // namespace messages
}  // namespace chatterino
