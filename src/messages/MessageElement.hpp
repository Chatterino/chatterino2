#pragma once

#include "common/FlagsEnum.hpp"
#include "messages/Link.hpp"
#include "messages/MessageColor.hpp"
#include "singletons/Fonts.hpp"
#include "src/messages/ImageSet.hpp"

#include <QRect>
#include <QString>
#include <QTime>
#include <boost/noncopyable.hpp>
#include <cstdint>
#include <memory>
#include <pajlada/signals/signalholder.hpp>
#include <vector>

namespace chatterino {
class Channel;
struct MessageLayoutContainer;
class MessageLayoutElement;

class Image;
using ImagePtr = std::shared_ptr<Image>;

struct Emote;
using EmotePtr = std::shared_ptr<const Emote>;

enum class MessageElementFlag : int64_t {
    None = 0LL,
    Misc = (1LL << 0),
    Text = (1LL << 1),

    Username = (1LL << 2),
    Timestamp = (1LL << 3),

    TwitchEmoteImage = (1LL << 4),
    TwitchEmoteText = (1LL << 5),
    TwitchEmote = TwitchEmoteImage | TwitchEmoteText,
    BttvEmoteImage = (1LL << 6),
    BttvEmoteText = (1LL << 7),
    BttvEmote = BttvEmoteImage | BttvEmoteText,

    ChannelPointReward = (1LL << 8),
    ChannelPointRewardImage = ChannelPointReward | TwitchEmoteImage,

    FfzEmoteImage = (1LL << 10),
    FfzEmoteText = (1LL << 11),
    FfzEmote = FfzEmoteImage | FfzEmoteText,
    EmoteImages = TwitchEmoteImage | BttvEmoteImage | FfzEmoteImage,
    EmoteText = TwitchEmoteText | BttvEmoteText | FfzEmoteText,

    BitsStatic = (1LL << 12),
    BitsAnimated = (1LL << 13),

    // Slot 1: Twitch
    // - Staff badge
    // - Admin badge
    // - Global Moderator badge
    BadgeGlobalAuthority = (1LL << 14),

    // Slot 2: Twitch
    // - Moderator badge
    // - Broadcaster badge
    BadgeChannelAuthority = (1LL << 15),

    // Slot 3: Twitch
    // - Subscription badges
    BadgeSubscription = (1LL << 16),

    // Slot 4: Twitch
    // - Turbo badge
    // - Prime badge
    // - Bit badges
    // - Game badges
    BadgeVanity = (1LL << 17),

    // Slot 5: Chatterino
    // - Chatterino developer badge
    // - Chatterino donator badge
    // - Chatterino top donator badge
    BadgeChatterino = (1LL << 18),

    // Slot 6: FrankerFaceZ
    // - FFZ developer badge
    // - FFZ bot badge
    // - FFZ donator badge
    BadgeFfz = (1LL << 32),

    Badges = BadgeGlobalAuthority | BadgeChannelAuthority | BadgeSubscription |
             BadgeVanity | BadgeChatterino | BadgeFfz,

    ChannelName = (1LL << 19),

    BitsAmount = (1LL << 20),

    ModeratorTools = (1LL << 21),

    EmojiImage = (1LL << 23),
    EmojiText = (1LL << 24),
    EmojiAll = EmojiImage | EmojiText,

    AlwaysShow = (1LL << 25),

    // used in the ChannelView class to make the collapse buttons visible if
    // needed
    Collapsed = (1LL << 26),

    // used for dynamic bold usernames
    BoldUsername = (1LL << 27),
    NonBoldUsername = (1LL << 28),

    // for links
    LowercaseLink = (1LL << 29),
    OriginalLink = (1LL << 30),

    // ZeroWidthEmotes are emotes that are supposed to overlay over any pre-existing emotes
    // e.g. BTTV's SoSnowy during christmas season
    ZeroWidthEmote = (1LL << 31),

    Default = Timestamp | Badges | Username | BitsStatic | FfzEmoteImage |
              BttvEmoteImage | TwitchEmoteImage | BitsAmount | Text |
              AlwaysShow,
};
using MessageElementFlags = FlagsEnum<MessageElementFlag>;

class MessageElement : boost::noncopyable
{
public:
    enum UpdateFlags : char {
        Update_Text = 1,
        Update_Emotes = 2,
        Update_Images = 4,
        Update_All = Update_Text | Update_Emotes | Update_Images
    };
    enum ThumbnailType : char {
        Link_Thumbnail = 1,
    };

    virtual ~MessageElement();

    MessageElement *setLink(const Link &link);
    MessageElement *setText(const QString &text);
    MessageElement *setTooltip(const QString &tooltip);
    MessageElement *setThumbnailType(const ThumbnailType type);
    MessageElement *setThumbnail(const ImagePtr &thumbnail);

    MessageElement *setTrailingSpace(bool value);
    const QString &getTooltip() const;
    const ImagePtr &getThumbnail() const;
    const ThumbnailType &getThumbnailType() const;

    const Link &getLink() const;
    bool hasTrailingSpace() const;
    MessageElementFlags getFlags() const;
    MessageElement *updateLink();

    virtual void addToContainer(MessageLayoutContainer &container,
                                MessageElementFlags flags) = 0;

    pajlada::Signals::NoArgSignal linkChanged;

protected:
    MessageElement(MessageElementFlags flags);
    bool trailingSpace = true;

private:
    QString text_;
    Link link_;
    QString tooltip_;
    ImagePtr thumbnail_;
    ThumbnailType thumbnailType_;
    MessageElementFlags flags_;
};

// used when layout element doesn't have a creator
class EmptyElement : public MessageElement
{
public:
    EmptyElement();

    void addToContainer(MessageLayoutContainer &container,
                        MessageElementFlags flags) override;

    static EmptyElement &instance();

private:
    ImagePtr image_;
};

// contains a simple image
class ImageElement : public MessageElement
{
public:
    ImageElement(ImagePtr image, MessageElementFlags flags);

    void addToContainer(MessageLayoutContainer &container,
                        MessageElementFlags flags) override;

private:
    ImagePtr image_;
};

// contains a text, it will split it into words
class TextElement : public MessageElement
{
public:
    TextElement(const QString &text, MessageElementFlags flags,
                const MessageColor &color = MessageColor::Text,
                FontStyle style = FontStyle::ChatMedium);
    ~TextElement() override = default;

    void addToContainer(MessageLayoutContainer &container,
                        MessageElementFlags flags) override;

private:
    MessageColor color_;
    FontStyle style_;

    struct Word {
        QString text;
        int width = -1;
    };
    std::vector<Word> words_;
};

// contains emote data and will pick the emote based on :
//   a) are images for the emote type enabled
//   b) which size it wants
class EmoteElement : public MessageElement
{
public:
    EmoteElement(const EmotePtr &data, MessageElementFlags flags_);

    void addToContainer(MessageLayoutContainer &container,
                        MessageElementFlags flags_) override;
    EmotePtr getEmote() const;

protected:
    virtual MessageLayoutElement *makeImageLayoutElement(const ImagePtr &image,
                                                         const QSize &size);

private:
    std::unique_ptr<TextElement> textElement_;
    EmotePtr emote_;
};

class BadgeElement : public MessageElement
{
public:
    BadgeElement(const EmotePtr &data, MessageElementFlags flags_);

    void addToContainer(MessageLayoutContainer &container,
                        MessageElementFlags flags_) override;

    EmotePtr getEmote() const;

protected:
    virtual MessageLayoutElement *makeImageLayoutElement(const ImagePtr &image,
                                                         const QSize &size);

private:
    EmotePtr emote_;
};

class ModBadgeElement : public BadgeElement
{
public:
    ModBadgeElement(const EmotePtr &data, MessageElementFlags flags_);

protected:
    MessageLayoutElement *makeImageLayoutElement(const ImagePtr &image,
                                                 const QSize &size) override;
};

// contains a text, formated depending on the preferences
class TimestampElement : public MessageElement
{
public:
    TimestampElement(QTime time_ = QTime::currentTime());
    ~TimestampElement() override = default;

    void addToContainer(MessageLayoutContainer &container,
                        MessageElementFlags flags) override;

    TextElement *formatTime(const QTime &time);

private:
    QTime time_;
    std::unique_ptr<TextElement> element_;
    QString format_;
};

// adds all the custom moderation buttons, adds a variable amount of items
// depending on settings fourtf: implement
class TwitchModerationElement : public MessageElement
{
public:
    TwitchModerationElement();

    void addToContainer(MessageLayoutContainer &container,
                        MessageElementFlags flags) override;
};

// contains a full message string that's split into words on space and parses irc colors that are then put into segments
// these segments are later passed to "MultiColorTextLayoutElement" elements to be rendered :)
class IrcTextElement : public MessageElement
{
public:
    IrcTextElement(const QString &text, MessageElementFlags flags,
                   FontStyle style = FontStyle::ChatMedium);
    ~IrcTextElement() override = default;

    void addToContainer(MessageLayoutContainer &container,
                        MessageElementFlags flags) override;

private:
    FontStyle style_;

    struct Segment {
        QString text;
        int fg = -1;
        int bg = -1;
    };

    struct Word {
        QString text;
        int width = -1;
        std::vector<Segment> segments;
    };

    std::vector<Word> words_;
};

// Forces a linebreak
class LinebreakElement : public MessageElement
{
public:
    LinebreakElement(MessageElementFlags flags);

    void addToContainer(MessageLayoutContainer &container,
                        MessageElementFlags flags) override;
};

// Image element which will pick the quality of the image based on ui scale
class ScalingImageElement : public MessageElement
{
public:
    ScalingImageElement(ImageSet images, MessageElementFlags flags);

    void addToContainer(MessageLayoutContainer &container,
                        MessageElementFlags flags) override;

private:
    ImageSet images_;
};
}  // namespace chatterino
