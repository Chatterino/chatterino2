#pragma once

#include "messages/image.hpp"
#include "messages/link.hpp"
#include "messages/messagecolor.hpp"
#include "singletons/fontmanager.hpp"
#include "util/emotemap.hpp"

#include <QRect>
#include <QString>
#include <QTime>
#include <boost/noncopyable.hpp>

#include <cstdint>
#include <memory>

namespace chatterino {
class Channel;
namespace util {
struct EmoteData;
}  // namespace util
namespace messages {
namespace layouts {
struct MessageLayoutContainer;
}  // namespace layouts

using namespace chatterino::messages::layouts;

class MessageElement : boost::noncopyable
{
public:
    enum Flags : uint32_t {
        None = 0,
        Misc = (1 << 0),
        Text = (1 << 1),

        Username = (1 << 2),
        Timestamp = (1 << 3),

        TwitchEmoteImage = (1 << 4),
        TwitchEmoteText = (1 << 5),
        TwitchEmote = TwitchEmoteImage | TwitchEmoteText,
        BttvEmoteImage = (1 << 6),
        BttvEmoteText = (1 << 7),
        BttvEmote = BttvEmoteImage | BttvEmoteText,
        FfzEmoteImage = (1 << 10),
        FfzEmoteText = (1 << 11),
        FfzEmote = FfzEmoteImage | FfzEmoteText,
        EmoteImages = TwitchEmoteImage | BttvEmoteImage | FfzEmoteImage,

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

        ChannelName = (1 << 19),

        BitsAmount = (1 << 20),

        ModeratorTools = (1 << 21),

        EmojiImage = (1 << 23),
        EmojiText = (1 << 24),
        EmojiAll = EmojiImage | EmojiText,

        AlwaysShow = (1 << 25),

        // used in the ChannelView class to make the collapse buttons visible if needed
        Collapsed = (1 << 26),

        Default = Timestamp | Badges | Username | BitsStatic | FfzEmoteImage | BttvEmoteImage |
                  TwitchEmoteImage | BitsAmount | Text | AlwaysShow,
    };

    enum UpdateFlags : char {
        Update_Text,
        Update_Emotes,
        Update_Images,
        Update_All = Update_Text | Update_Emotes | Update_Images
    };

    virtual ~MessageElement();

    MessageElement *setLink(const Link &link);
    MessageElement *setTooltip(const QString &tooltip);
    MessageElement *setTrailingSpace(bool value);
    const QString &getTooltip() const;
    const Link &getLink() const;
    bool hasTrailingSpace() const;
    Flags getFlags() const;

    virtual void addToContainer(MessageLayoutContainer &container, MessageElement::Flags flags) = 0;

protected:
    MessageElement(Flags flags);
    bool trailingSpace = true;

private:
    Link link;
    QString tooltip;
    Flags flags;
};

// contains a simple image
class ImageElement : public MessageElement
{
    Image *image;

public:
    ImageElement(Image *image, MessageElement::Flags flags);

    void addToContainer(MessageLayoutContainer &container, MessageElement::Flags flags) override;
};

// contains a text, it will split it into words
class TextElement : public MessageElement
{
    MessageColor color;
    FontStyle style;

    struct Word {
        QString text;
        int width = -1;
    };
    std::vector<Word> words;

public:
    TextElement(const QString &text, MessageElement::Flags flags,
                const MessageColor &color = MessageColor::Text,
                FontStyle style = FontStyle::Medium);
    ~TextElement() override = default;

    void addToContainer(MessageLayoutContainer &container, MessageElement::Flags flags) override;
};

// contains emote data and will pick the emote based on :
//   a) are images for the emote type enabled
//   b) which size it wants
class EmoteElement : public MessageElement
{
    std::unique_ptr<TextElement> textElement;

public:
    EmoteElement(const util::EmoteData &data, MessageElement::Flags flags);
    ~EmoteElement() override = default;

    void addToContainer(MessageLayoutContainer &container, MessageElement::Flags flags) override;

    const util::EmoteData data;
};

// contains a text, formated depending on the preferences
class TimestampElement : public MessageElement
{
    QTime time;
    std::unique_ptr<TextElement> element;
    QString format;

public:
    TimestampElement(QTime time = QTime::currentTime());
    ~TimestampElement() override = default;

    void addToContainer(MessageLayoutContainer &container, MessageElement::Flags flags) override;

    TextElement *formatTime(const QTime &time);
};

// adds all the custom moderation buttons, adds a variable amount of items depending on settings
// fourtf: implement
class TwitchModerationElement : public MessageElement
{
public:
    TwitchModerationElement();

    void addToContainer(MessageLayoutContainer &container, MessageElement::Flags flags) override;
};

}  // namespace messages
}  // namespace chatterino
