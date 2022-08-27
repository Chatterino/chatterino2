#include "messages/MessageElement.hpp"

#include "Application.hpp"
#include "debug/Benchmark.hpp"
#include "messages/Emote.hpp"
#include "messages/layouts/MessageLayoutContainer.hpp"
#include "messages/layouts/MessageLayoutElement.hpp"
#include "providers/emoji/Emojis.hpp"
#include "singletons/Emotes.hpp"
#include "singletons/Settings.hpp"
#include "singletons/Theme.hpp"
#include "util/DebugCount.hpp"

namespace chatterino {

namespace {
    std::vector<ImagePtr> imageSetPriority(const ImageSet &imageSet)
    {
        // TODO: Update with Image4 for 7tv support
        std::vector<ImagePtr> images;
        images.reserve(3);
        auto i1 = imageSet.getImage1();
        auto i2 = imageSet.getImage2();
        auto i3 = imageSet.getImage3();
        if (i3 && !i3->isEmpty())
        {
            images.push_back(std::move(i3));
        }
        if (i2 && !i2->isEmpty())
        {
            images.push_back(std::move(i2));
        }
        if (i1 && !i1->isEmpty())
        {
            images.push_back(std::move(i1));
        }
        return images;
    }

    void addImageSetToContainer(MessageLayoutContainer &container,
                                MessageElement &element,
                                const ImageSet &imageSet, float scale)
    {
        if (!imageSet.anyExist())
        {
            return;
        }

        const auto priority = imageSetPriority(imageSet);
        if (priority.empty())
        {
            return;
        }

        auto size = imageSet.firstAvailableSize() * container.getScale();

        container.addElement(
            (new PriorityImageLayoutElement(element, priority, size))
                ->setLink(element.getLink()));
    }
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

CircularImageElement::CircularImageElement(ImagePtr image, int padding,
                                           QColor background,
                                           MessageElementFlags flags)
    : MessageElement(flags)
    , image_(image)
    , padding_(padding)
    , background_(background)
{
}

void CircularImageElement::addToContainer(MessageLayoutContainer &container,
                                          MessageElementFlags flags)
{
    if (flags.hasAny(this->getFlags()))
    {
        auto imgSize = QSize(this->image_->width(), this->image_->height()) *
                       container.getScale();

        container.addElement((new ImageWithCircleBackgroundLayoutElement(
                                  *this, this->image_, imgSize,
                                  this->background_, this->padding_))
                                 ->setLink(this->getLink()));
    }
}

// EMOTE
EmoteElement::EmoteElement(const EmotePtr &emote, MessageElementFlags flags,
                           const MessageColor &textElementColor)
    : MessageElement(flags)
    , emote_(emote)
{
    this->textElement_.reset(new TextElement(
        emote->getCopyString(), MessageElementFlag::Misc, textElementColor));

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
            auto overallScale =
                getSettings()->emoteScale.getValue() * container.getScale();
            addImageSetToContainer(container, *this, this->emote_->images,
                                   overallScale);
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
        addImageSetToContainer(container, *this, this->emote_->images,
                               container.getScale());
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

// VIP BADGE
VipBadgeElement::VipBadgeElement(const EmotePtr &data,
                                 MessageElementFlags flags_)
    : BadgeElement(data, flags_)
{
}

MessageLayoutElement *VipBadgeElement::makeImageLayoutElement(
    const ImagePtr &image, const QSize &size)
{
    auto element =
        (new ImageLayoutElement(*this, image, size))->setLink(this->getLink());

    return element;
}

// FFZ Badge
FfzBadgeElement::FfzBadgeElement(const EmotePtr &data,
                                 MessageElementFlags flags_, QColor color_)
    : BadgeElement(data, flags_)
    , color(std::move(color_))
{
}

MessageLayoutElement *FfzBadgeElement::makeImageLayoutElement(
    const ImagePtr &image, const QSize &size)
{
    auto element =
        (new ImageWithBackgroundLayoutElement(*this, image, size, this->color))
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
            word.width = metrics.horizontalAdvance(word.text);
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

            for (int i = 0; i < textLength; i++)
            {
                auto isSurrogate = text.size() > i + 1 &&
                                   QChar::isHighSurrogate(text[i].unicode());

                auto charWidth = isSurrogate
                                     ? metrics.horizontalAdvance(text.mid(i, 2))
                                     : metrics.horizontalAdvance(text[i]);

                if (!container.fitsInLine(width + charWidth))
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

SingleLineTextElement::SingleLineTextElement(const QString &text,
                                             MessageElementFlags flags,
                                             const MessageColor &color,
                                             FontStyle style)
    : MessageElement(flags)
    , color_(color)
    , style_(style)
{
    for (const auto &word : text.split(' '))
    {
        this->words_.push_back({word, -1});
    }
}

void SingleLineTextElement::addToContainer(MessageLayoutContainer &container,
                                           MessageElementFlags flags)
{
    auto app = getApp();

    if (flags.hasAny(this->getFlags()))
    {
        QFontMetrics metrics =
            app->fonts->getFontMetrics(this->style_, container.getScale());

        auto getTextLayoutElement = [&](QString text, int width,
                                        bool hasTrailingSpace) {
            auto color = this->color_.getColor(*app->themes);
            app->themes->normalizeColor(color);

            auto e = (new TextLayoutElement(
                          *this, text, QSize(width, metrics.height()), color,
                          this->style_, container.getScale()))
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

        static const auto ellipsis = QStringLiteral("...");
        auto addEllipsis = [&]() {
            int ellipsisSize = metrics.horizontalAdvance(ellipsis);
            container.addElementNoLineBreak(
                getTextLayoutElement(ellipsis, ellipsisSize, false));
        };

        for (Word &word : this->words_)
        {
            auto parsedWords = app->emotes->emojis.parse(word.text);
            if (parsedWords.size() == 0)
            {
                continue;  // sanity check
            }

            auto &parsedWord = parsedWords[0];
            if (parsedWord.type() == typeid(EmotePtr))
            {
                auto emote = boost::get<EmotePtr>(parsedWord);
                if (emote->images.anyExist())
                {
                    const auto priority = imageSetPriority(emote->images);
                    if (priority.empty())
                    {
                        return;
                    }

                    auto overallScale = getSettings()->emoteScale.getValue() *
                                        container.getScale();
                    auto size =
                        emote->images.firstAvailableSize() * overallScale;

                    if (!container.fitsInLine(size.width()))
                    {
                        addEllipsis();
                        break;
                    }

                    container.addElementNoLineBreak(
                        (new PriorityImageLayoutElement(*this, priority, size))
                            ->setLink(this->getLink()));
                }
            }
            else if (parsedWord.type() == typeid(QString))
            {
                word.width = metrics.horizontalAdvance(word.text);

                // see if the text fits in the current line
                if (container.fitsInLine(word.width))
                {
                    container.addElementNoLineBreak(getTextLayoutElement(
                        word.text, word.width, this->hasTrailingSpace()));
                }
                else
                {
                    // word overflows, try minimum truncation
                    bool cutSuccess = false;
                    for (size_t cut = 1; cut < word.text.length(); ++cut)
                    {
                        // Cut off n characters and append the ellipsis.
                        // Try removing characters one by one until the word fits.
                        QString truncatedWord =
                            word.text.chopped(cut) + ellipsis;
                        int newSize = metrics.horizontalAdvance(truncatedWord);
                        if (container.fitsInLine(newSize))
                        {
                            container.addElementNoLineBreak(
                                getTextLayoutElement(truncatedWord, newSize,
                                                     false));
                            cutSuccess = true;
                            break;
                        }
                    }

                    if (!cutSuccess)
                    {
                        // We weren't able to show any part of the current word, so
                        // just append the ellipsis.
                        addEllipsis();
                    }

                    break;
                }
            }
        }

        container.breakLine();
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
        addImageSetToContainer(container, *this, this->images_,
                               container.getScale());
    }
}

ReplyCurveElement::ReplyCurveElement()
    : MessageElement(MessageElementFlag::RepliedMessage)
    // these values nicely align with a single badge
    , neededMargin_(3)
    , size_(18, 14)
{
}

void ReplyCurveElement::addToContainer(MessageLayoutContainer &container,
                                       MessageElementFlags flags)
{
    if (flags.hasAny(this->getFlags()))
    {
        QSize boxSize = this->size_ * container.getScale();
        container.addElement(new ReplyCurveLayoutElement(
            *this, boxSize, 1.5 * container.getScale(),
            this->neededMargin_ * container.getScale()));
    }
}

}  // namespace chatterino
