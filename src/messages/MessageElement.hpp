#pragma once

#include "common/FlagsEnum.hpp"
#include "messages/ImageSet.hpp"
#include "messages/Link.hpp"
#include "messages/MessageColor.hpp"
#include "singletons/Fonts.hpp"

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

    FfzEmoteImage = (1LL << 9),
    FfzEmoteText = (1LL << 10),
    FfzEmote = FfzEmoteImage | FfzEmoteText,

    SevenTVEmoteImage = (1LL << 34),
    SevenTVEmoteText = (1LL << 35),
    SevenTVEmote = SevenTVEmoteImage | SevenTVEmoteText,

    EmoteImages =
        TwitchEmoteImage | BttvEmoteImage | FfzEmoteImage | SevenTVEmoteImage,
    EmoteText =
        TwitchEmoteText | BttvEmoteText | FfzEmoteText | SevenTVEmoteText,

    BitsStatic = (1LL << 11),
    BitsAnimated = (1LL << 12),

    // Slot 1: Twitch
    // - Staff badge
    // - Admin badge
    // - Global Moderator badge
    BadgeGlobalAuthority = (1LL << 13),

    // Slot 2: Twitch
    // - Predictions badge
    BadgePredictions = (1LL << 14),

    // Slot 3: Twitch
    // - VIP badge
    // - Moderator badge
    // - Broadcaster badge
    BadgeChannelAuthority = (1LL << 15),

    // Slot 4: Twitch
    // - Subscription badges
    BadgeSubscription = (1LL << 16),

    // Slot 5: Twitch
    // - Turbo badge
    // - Prime badge
    // - Bit badges
    // - Game badges
    BadgeVanity = (1LL << 17),

    // Slot 6: Chatterino
    // - Chatterino developer badge
    // - Chatterino contributor badge
    // - Chatterino donator badge
    // - Chatterino top donator badge
    // - Chatterino special pepe badge
    // - Chatterino gnome badge
    BadgeChatterino = (1LL << 18),

    // Slot 7: 7TV
    // - 7TV Admin
    // - 7TV Dungeon Mistress
    // - 7TV Moderator
    // - 7TV Subscriber
    // - 7TV Translator
    // - 7TV Contributor
    BadgeSevenTV = (1LL << 36),

    // Slot 7: FrankerFaceZ
    // - FFZ developer badge
    // - FFZ bot badge
    // - FFZ donator badge
    BadgeFfz = (1LL << 19),

    Badges = BadgeGlobalAuthority | BadgePredictions | BadgeChannelAuthority |
             BadgeSubscription | BadgeVanity | BadgeChatterino | BadgeSevenTV |
             BadgeFfz,

    ChannelName = (1LL << 20),

    BitsAmount = (1LL << 21),

    ModeratorTools = (1LL << 22),

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
    // e.g. BTTV's SoSnowy during christmas season or 7TV's RainTime
    ZeroWidthEmote = (1LL << 31),

    // for elements of the message reply
    RepliedMessage = (1LL << 32),

    // for the reply button element
    ReplyButton = (1LL << 33),

    // (1LL << 34) through (1LL << 36) are occupied by
    // SevenTVEmoteImage, SevenTVEmoteText, and BadgeSevenTV,

    Default = Timestamp | Badges | Username | BitsStatic | FfzEmoteImage |
              BttvEmoteImage | SevenTVEmoteImage | TwitchEmoteImage |
              BitsAmount | Text | AlwaysShow,
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

// contains a image with a circular background color
class CircularImageElement : public MessageElement
{
public:
    CircularImageElement(ImagePtr image, int padding, QColor background,
                         MessageElementFlags flags);

    void addToContainer(MessageLayoutContainer &container,
                        MessageElementFlags flags) override;

private:
    ImagePtr image_;
    int padding_;
    QColor background_;
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

// contains a text that will be truncated to one line
class SingleLineTextElement : public MessageElement
{
public:
    SingleLineTextElement(const QString &text, MessageElementFlags flags,
                          const MessageColor &color = MessageColor::Text,
                          FontStyle style = FontStyle::ChatMedium);
    ~SingleLineTextElement() override = default;

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
    EmoteElement(const EmotePtr &data, MessageElementFlags flags_,
                 const MessageColor &textElementColor = MessageColor::Text);

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

class VipBadgeElement : public BadgeElement
{
public:
    VipBadgeElement(const EmotePtr &data, MessageElementFlags flags_);

protected:
    MessageLayoutElement *makeImageLayoutElement(const ImagePtr &image,
                                                 const QSize &size) override;
};

class FfzBadgeElement : public BadgeElement
{
public:
    FfzBadgeElement(const EmotePtr &data, MessageElementFlags flags_,
                    QColor color_);

protected:
    MessageLayoutElement *makeImageLayoutElement(const ImagePtr &image,
                                                 const QSize &size) override;
    const QColor color;
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

class ReplyCurveElement : public MessageElement
{
public:
    ReplyCurveElement();

    void addToContainer(MessageLayoutContainer &container,
                        MessageElementFlags flags) override;

private:
    int neededMargin_;
    QSize size_;
};

}  // namespace chatterino
