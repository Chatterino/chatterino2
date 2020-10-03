#include "messages/MessageElement.hpp"

#include "Application.hpp"
#include "common/IrcColors.hpp"
#include "debug/Benchmark.hpp"
#include "messages/Emote.hpp"
#include "messages/layouts/MessageLayoutContainer.hpp"
#include "messages/layouts/MessageLayoutElement.hpp"
#include "singletons/Settings.hpp"
#include "singletons/Theme.hpp"
#include "util/DebugCount.hpp"

namespace chatterino {

namespace {

    QRegularExpression IRC_COLOR_PARSE_REGEX(
        "(\u0003(\\d{1,2})?(,(\\d{1,2}))?|\u000f)",
        QRegularExpression::UseUnicodePropertiesOption);

}  // namespace

MessageElement::MessageElement(MessageElementFlags flags)
    : flags_(flags)
{
    DebugCount::increase("message elements");
}

MessageElement::~MessageElement()
{
    DebugCount::decrease("message elements");
}

MessageElement *MessageElement::setLink(const Link &link)
{
    this->link_ = link;
    return this;
}

MessageElement *MessageElement::setText(const QString &text)
{
    this->text_ = text;
    return this;
}

MessageElement *MessageElement::setTooltip(const QString &tooltip)
{
    this->tooltip_ = tooltip;
    return this;
}

MessageElement *MessageElement::setThumbnail(const ImagePtr &thumbnail)
{
    this->thumbnail_ = thumbnail;
    return this;
}

MessageElement *MessageElement::setThumbnailType(const ThumbnailType type)
{
    this->thumbnailType_ = type;
    return this;
}

MessageElement *MessageElement::setTrailingSpace(bool value)
{
    this->trailingSpace = value;
    return this;
}

const QString &MessageElement::getTooltip() const
{
    return this->tooltip_;
}

const ImagePtr &MessageElement::getThumbnail() const
{
    return this->thumbnail_;
}

const MessageElement::ThumbnailType &MessageElement::getThumbnailType() const
{
    return this->thumbnailType_;
}

const Link &MessageElement::getLink() const
{
    return this->link_;
}

bool MessageElement::hasTrailingSpace() const
{
    return this->trailingSpace;
}

MessageElementFlags MessageElement::getFlags() const
{
    return this->flags_;
}

MessageElement *MessageElement::updateLink()
{
    this->linkChanged.invoke();
    return this;
}

// Empty
EmptyElement::EmptyElement()
    : MessageElement(MessageElementFlag::None)
{
}

void EmptyElement::addToContainer(MessageLayoutContainer &container,
                                  MessageElementFlags flags)
{
}

EmptyElement &EmptyElement::instance()
{
    static EmptyElement instance;
    return instance;
}

// IMAGE
ImageElement::ImageElement(ImagePtr image, MessageElementFlags flags)
    : MessageElement(flags)
    , image_(image)
{
    //    this->setTooltip(image->getTooltip());
}

void ImageElement::addToContainer(MessageLayoutContainer &container,
                                  MessageElementFlags flags)
{
    if (flags.hasAny(this->getFlags()))
    {
        auto size = QSize(this->image_->width() * container.getScale(),
                          this->image_->height() * container.getScale());

        container.addElement((new ImageLayoutElement(*this, this->image_, size))
                                 ->setLink(this->getLink()));
    }
}

// EMOTE
EmoteElement::EmoteElement(const EmotePtr &emote, MessageElementFlags flags)
    : MessageElement(flags)
    , emote_(emote)
{
    this->textElement_.reset(
        new TextElement(emote->getCopyString(), MessageElementFlag::Misc));

    this->setTooltip(emote->tooltip.string);
}

EmotePtr EmoteElement::getEmote() const
{
    return this->emote_;
}

void EmoteElement::addToContainer(MessageLayoutContainer &container,
                                  MessageElementFlags flags)
{
    if (flags.hasAny(this->getFlags()))
    {
        if (flags.has(MessageElementFlag::EmoteImages))
        {
            auto image =
                this->emote_->images.getImageOrLoaded(container.getScale());
            if (image->isEmpty())
                return;

            auto emoteScale = getSettings()->emoteScale.getValue();

            auto size =
                QSize(int(container.getScale() * image->width() * emoteScale),
                      int(container.getScale() * image->height() * emoteScale));

            container.addElement(this->makeImageLayoutElement(image, size)
                                     ->setLink(this->getLink()));
        }
        else
        {
            if (this->textElement_)
            {
                this->textElement_->addToContainer(container,
                                                   MessageElementFlag::Misc);
            }
        }
    }
}

MessageLayoutElement *EmoteElement::makeImageLayoutElement(
    const ImagePtr &image, const QSize &size)
{
    return new ImageLayoutElement(*this, image, size);
}

// BADGE
BadgeElement::BadgeElement(const EmotePtr &emote, MessageElementFlags flags)
    : MessageElement(flags)
    , emote_(emote)
{
    this->setTooltip(emote->tooltip.string);
}

void BadgeElement::addToContainer(MessageLayoutContainer &container,
                                  MessageElementFlags flags)
{
    if (flags.hasAny(this->getFlags()))
    {
        auto image =
            this->emote_->images.getImageOrLoaded(container.getScale());
        if (image->isEmpty())
            return;

        auto size = QSize(int(container.getScale() * image->width()),
                          int(container.getScale() * image->height()));

        container.addElement(this->makeImageLayoutElement(image, size));
    }
}

EmotePtr BadgeElement::getEmote() const
{
    return this->emote_;
}

MessageLayoutElement *BadgeElement::makeImageLayoutElement(
    const ImagePtr &image, const QSize &size)
{
    auto element =
        (new ImageLayoutElement(*this, image, size))->setLink(this->getLink());

    return element;
}

// MOD BADGE
ModBadgeElement::ModBadgeElement(const EmotePtr &data,
                                 MessageElementFlags flags_)
    : BadgeElement(data, flags_)
{
}

MessageLayoutElement *ModBadgeElement::makeImageLayoutElement(
    const ImagePtr &image, const QSize &size)
{
    static const QColor modBadgeBackgroundColor("#34AE0A");

    auto element = (new ImageWithBackgroundLayoutElement(
                        *this, image, size, modBadgeBackgroundColor))
                       ->setLink(this->getLink());

    return element;
}

// TEXT
TextElement::TextElement(const QString &text, MessageElementFlags flags,
                         const MessageColor &color, FontStyle style)
    : MessageElement(flags)
    , color_(color)
    , style_(style)
{
    for (const auto &word : text.split(' '))
    {
        this->words_.push_back({word, -1});
        // fourtf: add logic to store multiple spaces after message
    }
}

void TextElement::addToContainer(MessageLayoutContainer &container,
                                 MessageElementFlags flags)
{
    auto app = getApp();

    if (flags.hasAny(this->getFlags()))
    {
        QFontMetrics metrics =
            app->fonts->getFontMetrics(this->style_, container.getScale());

        for (Word &word : this->words_)
        {
            auto getTextLayoutElement = [&](QString text, int width,
                                            bool hasTrailingSpace) {
                auto color = this->color_.getColor(*app->themes);
                app->themes->normalizeColor(color);

                auto e = (new TextLayoutElement(
                              *this, text, QSize(width, metrics.height()),
                              color, this->style_, container.getScale()))
                             ->setLink(this->getLink());
                e->setTrailingSpace(hasTrailingSpace);
                e->setText(text);

                // If URL link was changed,
                // Should update it in MessageLayoutElement too!
                if (this->getLink().type == Link::Url)
                {
                    static_cast<TextLayoutElement *>(e)->listenToLinkChanges();
                }
                return e;
            };

            // fourtf: add again
            //            if (word.width == -1) {
            word.width = metrics.width(word.text);
            //            }

            // see if the text fits in the current line
            if (container.fitsInLine(word.width))
            {
                container.addElementNoLineBreak(getTextLayoutElement(
                    word.text, word.width, this->hasTrailingSpace()));
                continue;
            }

            // see if the text fits in the next line
            if (!container.atStartOfLine())
            {
                container.breakLine();

                if (container.fitsInLine(word.width))
                {
                    container.addElementNoLineBreak(getTextLayoutElement(
                        word.text, word.width, this->hasTrailingSpace()));
                    continue;
                }
            }

            // we done goofed, we need to wrap the text
            QString text = word.text;
            int textLength = text.length();
            int wordStart = 0;
            int width = 0;

            // QChar::isHighSurrogate(text[0].unicode()) ? 2 : 1

            for (int i = 0; i < textLength; i++)  //
            {
                auto isSurrogate = text.size() > i + 1 &&
                                   QChar::isHighSurrogate(text[i].unicode());

                auto charWidth = isSurrogate ? metrics.width(text.mid(i, 2))
                                             : metrics.width(text[i]);

                if (!container.fitsInLine(width + charWidth))  //
                {
                    container.addElementNoLineBreak(getTextLayoutElement(
                        text.mid(wordStart, i - wordStart), width, false));
                    container.breakLine();

                    wordStart = i;
                    width = charWidth;

                    if (isSurrogate)
                        i++;
                    continue;
                }

                width += charWidth;

                if (isSurrogate)
                    i++;
            }
            //add the final piece of wrapped text
            container.addElementNoLineBreak(getTextLayoutElement(
                text.mid(wordStart), width, this->hasTrailingSpace()));
        }
    }
}

// TIMESTAMP
TimestampElement::TimestampElement(QTime time)
    : MessageElement(MessageElementFlag::Timestamp)
    , time_(time)
    , element_(this->formatTime(time))
{
    assert(this->element_ != nullptr);
}

void TimestampElement::addToContainer(MessageLayoutContainer &container,
                                      MessageElementFlags flags)
{
    if (flags.hasAny(this->getFlags()))
    {
        if (getSettings()->timestampFormat != this->format_)
        {
            this->format_ = getSettings()->timestampFormat.getValue();
            this->element_.reset(this->formatTime(this->time_));
        }

        this->element_->addToContainer(container, flags);
    }
}

TextElement *TimestampElement::formatTime(const QTime &time)
{
    static QLocale locale("en_US");

    QString format = locale.toString(time, getSettings()->timestampFormat);

    return new TextElement(format, MessageElementFlag::Timestamp,
                           MessageColor::System, FontStyle::ChatMedium);
}

QTime TimestampElement::getTime()
{
    return this->time_;
}

// TWITCH MODERATION
TwitchModerationElement::TwitchModerationElement()
    : MessageElement(MessageElementFlag::ModeratorTools)
{
}

void TwitchModerationElement::addToContainer(MessageLayoutContainer &container,
                                             MessageElementFlags flags)
{
    if (flags.has(MessageElementFlag::ModeratorTools))
    {
        QSize size(int(container.getScale() * 16),
                   int(container.getScale() * 16));
        auto actions = getCSettings().moderationActions.readOnly();
        for (const auto &action : *actions)
        {
            if (auto image = action.getImage())
            {
                container.addElement(
                    (new ImageLayoutElement(*this, image.get(), size))
                        ->setLink(Link(Link::UserAction, action.getAction())));
            }
            else
            {
                container.addElement(
                    (new TextIconLayoutElement(*this, action.getLine1(),
                                               action.getLine2(),
                                               container.getScale(), size))
                        ->setLink(Link(Link::UserAction, action.getAction())));
            }
        }
    }
}

// TEXT
// IrcTextElement gets its color from the color code in the message, and can change from character to character.
// This differs from the TextElement
IrcTextElement::IrcTextElement(const QString &fullText,
                               MessageElementFlags flags, FontStyle style)
    : MessageElement(flags)
    , style_(style)
{
    assert(IRC_COLOR_PARSE_REGEX.isValid());

    // Default pen colors. -1 = default theme colors
    int fg = -1, bg = -1;

    // Split up the message in words (space separated)
    // Each word contains one or more colored segments.
    // The color of that segment is "global", as in it can be decided by the word before it.
    for (const auto &text : fullText.split(' '))
    {
        std::vector<Segment> segments;

        int pos = 0;
        int lastPos = 0;

        auto i = IRC_COLOR_PARSE_REGEX.globalMatch(text);

        while (i.hasNext())
        {
            auto match = i.next();

            if (lastPos != match.capturedStart() && match.capturedStart() != 0)
            {
                auto seg = Segment{};
                seg.text = text.mid(lastPos, match.capturedStart() - lastPos);
                seg.fg = fg;
                seg.bg = bg;
                segments.emplace_back(seg);
                lastPos = match.capturedStart() + match.capturedLength();
            }
            if (!match.captured(1).isEmpty())
            {
                fg = -1;
                bg = -1;
            }

            if (!match.captured(2).isEmpty())
            {
                fg = match.captured(2).toInt(nullptr);
            }
            else
            {
                fg = -1;
            }
            if (!match.captured(4).isEmpty())
            {
                bg = match.captured(4).toInt(nullptr);
            }
            else if (fg == -1)
            {
                bg = -1;
            }

            lastPos = match.capturedStart() + match.capturedLength();
        }

        auto seg = Segment{};
        seg.text = text.mid(lastPos);
        seg.fg = fg;
        seg.bg = bg;
        segments.emplace_back(seg);

        QString n(text);

        n.replace(IRC_COLOR_PARSE_REGEX, "");

        Word w{
            n,
            -1,
            segments,
        };
        this->words_.emplace_back(w);
    }
}

void IrcTextElement::addToContainer(MessageLayoutContainer &container,
                                    MessageElementFlags flags)
{
    auto app = getApp();

    MessageColor defaultColorType = MessageColor::Text;
    auto defaultColor = defaultColorType.getColor(*app->themes);
    if (flags.hasAny(this->getFlags()))
    {
        QFontMetrics metrics =
            app->fonts->getFontMetrics(this->style_, container.getScale());

        for (auto &word : this->words_)
        {
            auto getTextLayoutElement = [&](QString text,
                                            std::vector<Segment> segments,
                                            int width, bool hasTrailingSpace) {
                std::vector<PajSegment> xd{};

                for (const auto &segment : segments)
                {
                    QColor color = defaultColor;
                    if (segment.fg >= 0 && segment.fg <= 98)
                    {
                        color = IRC_COLORS[segment.fg];
                    }
                    app->themes->normalizeColor(color);
                    xd.emplace_back(PajSegment{segment.text, color});
                }

                auto e = (new MultiColorTextLayoutElement(
                              *this, text, QSize(width, metrics.height()), xd,
                              this->style_, container.getScale()))
                             ->setLink(this->getLink());
                e->setTrailingSpace(true);
                e->setText(text);

                // If URL link was changed,
                // Should update it in MessageLayoutElement too!
                if (this->getLink().type == Link::Url)
                {
                    static_cast<TextLayoutElement *>(e)->listenToLinkChanges();
                }
                return e;
            };

            // fourtf: add again
            //            if (word.width == -1) {
            word.width = metrics.width(word.text);
            //            }

            // see if the text fits in the current line
            if (container.fitsInLine(word.width))
            {
                container.addElementNoLineBreak(
                    getTextLayoutElement(word.text, word.segments, word.width,
                                         this->hasTrailingSpace()));
                continue;
            }

            // see if the text fits in the next line
            if (!container.atStartOfLine())
            {
                container.breakLine();

                if (container.fitsInLine(word.width))
                {
                    container.addElementNoLineBreak(getTextLayoutElement(
                        word.text, word.segments, word.width,
                        this->hasTrailingSpace()));
                    continue;
                }
            }

            // we done goofed, we need to wrap the text
            QString text = word.text;
            std::vector<Segment> segments = word.segments;
            int textLength = text.length();
            int wordStart = 0;
            int width = 0;

            // QChar::isHighSurrogate(text[0].unicode()) ? 2 : 1

            // XXX(pajlada): NOT TESTED
            for (int i = 0; i < textLength; i++)  //
            {
                auto isSurrogate = text.size() > i + 1 &&
                                   QChar::isHighSurrogate(text[i].unicode());

                auto charWidth = isSurrogate ? metrics.width(text.mid(i, 2))
                                             : metrics.width(text[i]);

                if (!container.fitsInLine(width + charWidth))
                {
                    std::vector<Segment> pieceSegments;
                    int charactersLeft = i - wordStart;
                    assert(charactersLeft > 0);
                    for (auto segmentIt = segments.begin();
                         segmentIt != segments.end();)
                    {
                        assert(charactersLeft > 0);
                        auto &segment = *segmentIt;
                        if (charactersLeft >= segment.text.length())
                        {
                            // Entire segment fits in this piece
                            pieceSegments.push_back(segment);
                            charactersLeft -= segment.text.length();
                            segmentIt = segments.erase(segmentIt);

                            assert(charactersLeft >= 0);

                            if (charactersLeft == 0)
                            {
                                break;
                            }
                        }
                        else
                        {
                            // Only part of the segment fits in this piece
                            // We create a new segment with the characters that fit, and modify the segment we checked to only contain the characters we didn't consume
                            Segment segmentThatFitsInPiece{
                                segment.text.left(charactersLeft), segment.fg,
                                segment.bg};
                            pieceSegments.emplace_back(segmentThatFitsInPiece);
                            segment.text = segment.text.mid(charactersLeft);

                            break;
                        }
                    }

                    container.addElementNoLineBreak(
                        getTextLayoutElement(text.mid(wordStart, i - wordStart),
                                             pieceSegments, width, false));
                    container.breakLine();

                    wordStart = i;
                    width = charWidth;

                    if (isSurrogate)
                        i++;
                    continue;
                }

                width += charWidth;

                if (isSurrogate)
                    i++;
            }

            // Add last remaining text & segments
            container.addElementNoLineBreak(
                getTextLayoutElement(text.mid(wordStart), segments, width,
                                     this->hasTrailingSpace()));
        }
    }
}

LinebreakElement::LinebreakElement(MessageElementFlags flags)
    : MessageElement(flags)
{
}

void LinebreakElement::addToContainer(MessageLayoutContainer &container,
                                      MessageElementFlags flags)
{
    if (flags.hasAny(this->getFlags()))
    {
        container.breakLine();
    }
}

ScalingImageElement::ScalingImageElement(ImageSet images,
                                         MessageElementFlags flags)
    : MessageElement(flags)
    , images_(images)
{
}

void ScalingImageElement::addToContainer(MessageLayoutContainer &container,
                                         MessageElementFlags flags)
{
    if (flags.hasAny(this->getFlags()))
    {
        const auto &image =
            this->images_.getImageOrLoaded(container.getScale());
        if (image->isEmpty())
            return;

        auto size = QSize(image->width() * container.getScale(),
                          image->height() * container.getScale());

        container.addElement((new ImageLayoutElement(*this, image, size))
                                 ->setLink(this->getLink()));
    }
}

}  // namespace chatterino
