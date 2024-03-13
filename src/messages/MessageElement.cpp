#include "messages/MessageElement.hpp"

#include "Application.hpp"
#include "controllers/moderationactions/ModerationAction.hpp"
#include "debug/Benchmark.hpp"
#include "messages/Emote.hpp"
#include "messages/Image.hpp"
#include "messages/layouts/MessageLayoutContainer.hpp"
#include "messages/layouts/MessageLayoutElement.hpp"
#include "providers/emoji/Emojis.hpp"
#include "singletons/Emotes.hpp"
#include "singletons/Settings.hpp"
#include "singletons/Theme.hpp"
#include "util/DebugCount.hpp"
#include "util/Variant.hpp"

namespace chatterino {

namespace {

    // Computes the bounding box for the given vector of images
    QSize getBoundingBoxSize(const std::vector<ImagePtr> &images)
    {
        int width = 0;
        int height = 0;
        for (const auto &img : images)
        {
            width = std::max(width, img->width());
            height = std::max(height, img->height());
        }

        return QSize(width, height);
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

MessageElement *MessageElement::setTooltip(const QString &tooltip)
{
    this->tooltip_ = tooltip;
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

Link MessageElement::getLink() const
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

void MessageElement::addFlags(MessageElementFlags flags)
{
    this->flags_.set(flags);
}

// IMAGE
ImageElement::ImageElement(ImagePtr image, MessageElementFlags flags)
    : MessageElement(flags)
    , image_(std::move(image))
{
}

void ImageElement::addToContainer(MessageLayoutContainer &container,
                                  MessageElementFlags flags)
{
    if (flags.hasAny(this->getFlags()))
    {
        auto size = QSize(this->image_->width() * container.getScale(),
                          this->image_->height() * container.getScale());

        container.addElement(
            (new ImageLayoutElement(*this, this->image_, size)));
    }
}

CircularImageElement::CircularImageElement(ImagePtr image, int padding,
                                           QColor background,
                                           MessageElementFlags flags)
    : MessageElement(flags)
    , image_(std::move(image))
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

        container.addElement(new ImageWithCircleBackgroundLayoutElement(
            *this, this->image_, imgSize, this->background_, this->padding_));
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
            auto image =
                this->emote_->images.getImageOrLoaded(container.getScale());
            if (image->isEmpty())
            {
                return;
            }

            auto emoteScale = getSettings()->emoteScale.getValue();

            auto size =
                QSize(int(container.getScale() * image->width() * emoteScale),
                      int(container.getScale() * image->height() * emoteScale));

            container.addElement(this->makeImageLayoutElement(image, size));
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

LayeredEmoteElement::LayeredEmoteElement(
    std::vector<LayeredEmoteElement::Emote> &&emotes, MessageElementFlags flags,
    const MessageColor &textElementColor)
    : MessageElement(flags)
    , emotes_(std::move(emotes))
    , textElementColor_(textElementColor)
{
    this->updateTooltips();
}

void LayeredEmoteElement::addEmoteLayer(const LayeredEmoteElement::Emote &emote)
{
    this->emotes_.push_back(emote);
    this->updateTooltips();
}

void LayeredEmoteElement::addToContainer(MessageLayoutContainer &container,
                                         MessageElementFlags flags)
{
    if (flags.hasAny(this->getFlags()))
    {
        if (flags.has(MessageElementFlag::EmoteImages))
        {
            auto images = this->getLoadedImages(container.getScale());
            if (images.empty())
            {
                return;
            }

            auto emoteScale = getSettings()->emoteScale.getValue();
            float overallScale = emoteScale * container.getScale();

            auto largestSize = getBoundingBoxSize(images) * overallScale;
            std::vector<QSize> individualSizes;
            individualSizes.reserve(this->emotes_.size());
            for (auto img : images)
            {
                individualSizes.push_back(QSize(img->width(), img->height()) *
                                          overallScale);
            }

            container.addElement(this->makeImageLayoutElement(
                images, individualSizes, largestSize));
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

std::vector<ImagePtr> LayeredEmoteElement::getLoadedImages(float scale)
{
    std::vector<ImagePtr> res;
    res.reserve(this->emotes_.size());

    for (const auto &emote : this->emotes_)
    {
        auto image = emote.ptr->images.getImageOrLoaded(scale);
        if (image->isEmpty())
        {
            continue;
        }
        res.push_back(image);
    }
    return res;
}

MessageLayoutElement *LayeredEmoteElement::makeImageLayoutElement(
    const std::vector<ImagePtr> &images, const std::vector<QSize> &sizes,
    QSize largestSize)
{
    return new LayeredImageLayoutElement(*this, images, sizes, largestSize);
}

void LayeredEmoteElement::updateTooltips()
{
    if (!this->emotes_.empty())
    {
        QString copyStr = this->getCopyString();
        this->textElement_.reset(new TextElement(
            copyStr, MessageElementFlag::Misc, this->textElementColor_));
        this->setTooltip(copyStr);
    }

    std::vector<QString> result;
    result.reserve(this->emotes_.size());

    for (const auto &emote : this->emotes_)
    {
        result.push_back(emote.ptr->tooltip.string);
    }

    this->emoteTooltips_ = std::move(result);
}

const std::vector<QString> &LayeredEmoteElement::getEmoteTooltips() const
{
    return this->emoteTooltips_;
}

QString LayeredEmoteElement::getCleanCopyString() const
{
    QString result;
    for (size_t i = 0; i < this->emotes_.size(); ++i)
    {
        if (i != 0)
        {
            result += " ";
        }
        result += TwitchEmotes::cleanUpEmoteCode(
            this->emotes_[i].ptr->getCopyString());
    }
    return result;
}

QString LayeredEmoteElement::getCopyString() const
{
    QString result;
    for (size_t i = 0; i < this->emotes_.size(); ++i)
    {
        if (i != 0)
        {
            result += " ";
        }
        result += this->emotes_[i].ptr->getCopyString();
    }
    return result;
}

const std::vector<LayeredEmoteElement::Emote> &LayeredEmoteElement::getEmotes()
    const
{
    return this->emotes_;
}

std::vector<LayeredEmoteElement::Emote> LayeredEmoteElement::getUniqueEmotes()
    const
{
    // Functor for std::copy_if that keeps track of seen elements
    struct NotDuplicate {
        bool operator()(const Emote &element)
        {
            return seen.insert(element.ptr).second;
        }

    private:
        std::set<EmotePtr> seen;
    };

    // Get unique emotes while maintaining relative layering order
    NotDuplicate dup;
    std::vector<Emote> unique;
    std::copy_if(this->emotes_.begin(), this->emotes_.end(),
                 std::back_insert_iterator(unique), dup);

    return unique;
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
        {
            return;
        }

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
    auto *element = new ImageLayoutElement(*this, image, size);

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

    auto *element = new ImageWithBackgroundLayoutElement(
        *this, image, size, modBadgeBackgroundColor);

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
    auto *element = new ImageLayoutElement(*this, image, size);

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
    auto *element =
        new ImageWithBackgroundLayoutElement(*this, image, size, this->color);

    return element;
}

// TEXT
TextElement::TextElement(const QString &text, MessageElementFlags flags,
                         const MessageColor &color, FontStyle style)
    : MessageElement(flags)
    , color_(color)
    , style_(style)
{
    this->words_ = text.split(' ');
    // fourtf: add logic to store multiple spaces after message
}

void TextElement::addToContainer(MessageLayoutContainer &container,
                                 MessageElementFlags flags)
{
    auto *app = getApp();

    if (flags.hasAny(this->getFlags()))
    {
        QFontMetrics metrics =
            app->getFonts()->getFontMetrics(this->style_, container.getScale());

        for (const auto &word : this->words_)
        {
            auto getTextLayoutElement = [&](QString text, int width,
                                            bool hasTrailingSpace) {
                auto color = this->color_.getColor(*app->getThemes());
                app->getThemes()->normalizeColor(color);

                auto *e = new TextLayoutElement(
                    *this, text, QSize(width, metrics.height()), color,
                    this->style_, container.getScale());
                e->setTrailingSpace(hasTrailingSpace);
                e->setText(text);

                return e;
            };

            auto width = metrics.horizontalAdvance(word);

            // see if the text fits in the current line
            if (container.fitsInLine(width))
            {
                container.addElementNoLineBreak(getTextLayoutElement(
                    word, width, this->hasTrailingSpace()));
                continue;
            }

            // see if the text fits in the next line
            if (!container.atStartOfLine())
            {
                container.breakLine();

                if (container.fitsInLine(width))
                {
                    container.addElementNoLineBreak(getTextLayoutElement(
                        word, width, this->hasTrailingSpace()));
                    continue;
                }
            }

            // we done goofed, we need to wrap the text
            auto textLength = word.length();
            int wordStart = 0;
            width = 0;

            // QChar::isHighSurrogate(text[0].unicode()) ? 2 : 1

            for (int i = 0; i < textLength; i++)
            {
                auto isSurrogate = word.size() > i + 1 &&
                                   QChar::isHighSurrogate(word[i].unicode());

                auto charWidth = isSurrogate
                                     ? metrics.horizontalAdvance(word.mid(i, 2))
                                     : metrics.horizontalAdvance(word[i]);

                if (!container.fitsInLine(width + charWidth))
                {
                    container.addElementNoLineBreak(getTextLayoutElement(
                        word.mid(wordStart, i - wordStart), width, false));
                    container.breakLine();

                    wordStart = i;
                    width = charWidth;

                    if (isSurrogate)
                    {
                        i++;
                    }
                    continue;
                }

                width += charWidth;

                if (isSurrogate)
                {
                    i++;
                }
            }
            //add the final piece of wrapped text
            container.addElementNoLineBreak(getTextLayoutElement(
                word.mid(wordStart), width, this->hasTrailingSpace()));
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
    auto *app = getApp();

    if (flags.hasAny(this->getFlags()))
    {
        QFontMetrics metrics =
            app->getFonts()->getFontMetrics(this->style_, container.getScale());

        auto getTextLayoutElement = [&](QString text, int width,
                                        bool hasTrailingSpace) {
            auto color = this->color_.getColor(*app->getThemes());
            app->getThemes()->normalizeColor(color);

            auto *e = new TextLayoutElement(
                *this, text, QSize(width, metrics.height()), color,
                this->style_, container.getScale());
            e->setTrailingSpace(hasTrailingSpace);
            e->setText(text);

            return e;
        };

        static const auto ellipsis = QStringLiteral("…");

        // String to continuously append words onto until we place it in the container
        // once we encounter an emote or reach the end of the message text. */
        QString currentText;

        container.first = FirstWord::Neutral;

        bool firstIteration = true;
        for (Word &word : this->words_)
        {
            if (firstIteration)
            {
                firstIteration = false;
            }
            else
            {
                currentText += ' ';
            }

            bool done = false;
            for (const auto &parsedWord :
                 app->getEmotes()->getEmojis()->parse(word.text))
            {
                if (parsedWord.type() == typeid(QString))
                {
                    currentText += boost::get<QString>(parsedWord);
                    QString prev =
                        currentText;  // only increments the ref-count
                    currentText =
                        metrics.elidedText(currentText, Qt::ElideRight,
                                           container.remainingWidth());
                    if (currentText != prev)
                    {
                        done = true;
                        break;
                    }
                }
                else if (parsedWord.type() == typeid(EmotePtr))
                {
                    auto emote = boost::get<EmotePtr>(parsedWord);
                    auto image =
                        emote->images.getImageOrLoaded(container.getScale());
                    if (!image->isEmpty())
                    {
                        auto emoteScale = getSettings()->emoteScale.getValue();

                        int currentWidth =
                            metrics.horizontalAdvance(currentText);
                        auto emoteSize =
                            QSize(image->width(), image->height()) *
                            (emoteScale * container.getScale());

                        if (!container.fitsInLine(currentWidth +
                                                  emoteSize.width()))
                        {
                            currentText += ellipsis;
                            done = true;
                            break;
                        }

                        // Add currently pending text to container, then add the emote after.
                        container.addElementNoLineBreak(getTextLayoutElement(
                            currentText, currentWidth, false));
                        currentText.clear();

                        container.addElementNoLineBreak(
                            (new ImageLayoutElement(*this, image, emoteSize))
                                ->setLink(this->getLink())
                                ->setTrailingSpace(false));
                    }
                }
            }

            if (done)
            {
                break;
            }
        }

        // Add the last of the pending message text to the container.
        if (!currentText.isEmpty())
        {
            int width = metrics.horizontalAdvance(currentText);
            container.addElementNoLineBreak(
                getTextLayoutElement(currentText, width, false));
        }

        container.breakLine();
    }
}

LinkElement::LinkElement(const Parsed &parsed, MessageElementFlags flags,
                         const MessageColor &color, FontStyle style)
    : TextElement({}, flags, color, style)
    , linkInfo_(parsed.original)
    , lowercase_({parsed.lowercase})
    , original_({parsed.original})
{
    this->setTooltip(parsed.original);
}

void LinkElement::addToContainer(MessageLayoutContainer &container,
                                 MessageElementFlags flags)
{
    this->words_ =
        getSettings()->lowercaseDomains ? this->lowercase_ : this->original_;
    TextElement::addToContainer(container, flags);
}

Link LinkElement::getLink() const
{
    return {Link::Url, this->linkInfo_.url()};
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
        auto actions = getSettings()->moderationActions.readOnly();
        for (const auto &action : *actions)
        {
            if (auto image = action.getImage())
            {
                container.addElement(
                    (new ImageLayoutElement(*this, *image, size))
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
    , images_(std::move(images))
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
        {
            return;
        }

        auto size = QSize(image->width() * container.getScale(),
                          image->height() * container.getScale());

        container.addElement(new ImageLayoutElement(*this, image, size));
    }
}

ReplyCurveElement::ReplyCurveElement()
    : MessageElement(MessageElementFlag::RepliedMessage)
{
}

void ReplyCurveElement::addToContainer(MessageLayoutContainer &container,
                                       MessageElementFlags flags)
{
    static const int width = 18;         // Overall width
    static const float thickness = 1.5;  // Pen width
    static const int radius = 6;         // Radius of the top left corner
    static const int margin = 2;         // Top/Left/Bottom margin

    if (flags.hasAny(this->getFlags()))
    {
        float scale = container.getScale();
        container.addElement(
            new ReplyCurveLayoutElement(*this, width * scale, thickness * scale,
                                        radius * scale, margin * scale));
    }
}

}  // namespace chatterino
