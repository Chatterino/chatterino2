#include "messages/MessageElement.hpp"

#include "Application.hpp"
#include "common/Literals.hpp"
#include "controllers/moderationactions/ModerationAction.hpp"
#include "debug/Benchmark.hpp"
#include "messages/Emote.hpp"
#include "messages/Image.hpp"
#include "messages/layouts/MessageLayoutContainer.hpp"
#include "messages/layouts/MessageLayoutContext.hpp"
#include "messages/layouts/MessageLayoutElement.hpp"
#include "providers/emoji/Emojis.hpp"
#include "singletons/Emotes.hpp"
#include "singletons/Settings.hpp"
#include "singletons/Theme.hpp"
#include "util/DebugCount.hpp"
#include "util/Variant.hpp"

#include <QJsonArray>
#include <QJsonObject>
#include <QJsonValue>

namespace chatterino {

using namespace literals;

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

QJsonObject MessageElement::toJson() const
{
    return {
        {"trailingSpace"_L1, this->trailingSpace},
        {
            "link"_L1,
            {{
                {"type"_L1, qmagicenum::enumNameString(this->link_.type)},
                {"value"_L1, this->link_.value},
            }},
        },
        {"tooltip"_L1, this->tooltip_},
        {"flags"_L1, qmagicenum::enumFlagsName(this->flags_.value())},
    };
}

// IMAGE
ImageElement::ImageElement(ImagePtr image, MessageElementFlags flags)
    : MessageElement(flags)
    , image_(std::move(image))
{
}

void ImageElement::addToContainer(MessageLayoutContainer &container,
                                  const MessageLayoutContext &ctx)
{
    if (ctx.flags.hasAny(this->getFlags()))
    {
        auto size = QSize(this->image_->width() * container.getScale(),
                          this->image_->height() * container.getScale());

        container.addElement(
            (new ImageLayoutElement(*this, this->image_, size)));
    }
}

QJsonObject ImageElement::toJson() const
{
    auto base = MessageElement::toJson();
    base["type"_L1] = u"ImageElement"_s;
    base["url"_L1] = this->image_->url().string;

    return base;
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
                                          const MessageLayoutContext &ctx)
{
    if (ctx.flags.hasAny(this->getFlags()))
    {
        auto imgSize = QSize(this->image_->width(), this->image_->height()) *
                       container.getScale();

        container.addElement(new ImageWithCircleBackgroundLayoutElement(
            *this, this->image_, imgSize, this->background_, this->padding_));
    }
}

QJsonObject CircularImageElement::toJson() const
{
    auto base = MessageElement::toJson();
    base["type"_L1] = u"CircularImageElement"_s;
    base["url"_L1] = this->image_->url().string;
    base["padding"_L1] = this->padding_;
    base["background"_L1] = this->background_.name(QColor::HexArgb);

    return base;
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
                                  const MessageLayoutContext &ctx)
{
    if (ctx.flags.hasAny(this->getFlags()))
    {
        if (ctx.flags.has(MessageElementFlag::EmoteImages))
        {
            auto image = this->emote_->images.getImageOrLoaded(
                container.getImageScale());
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
                auto textCtx = ctx;
                textCtx.flags = MessageElementFlag::Misc;
                this->textElement_->addToContainer(container, textCtx);
            }
        }
    }
}

MessageLayoutElement *EmoteElement::makeImageLayoutElement(
    const ImagePtr &image, const QSize &size)
{
    return new ImageLayoutElement(*this, image, size);
}

QJsonObject EmoteElement::toJson() const
{
    auto base = MessageElement::toJson();
    base["type"_L1] = u"EmoteElement"_s;
    base["emote"_L1] = this->emote_->toJson();
    if (this->textElement_)
    {
        base["text"_L1] = this->textElement_->toJson();
    }

    return base;
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
                                         const MessageLayoutContext &ctx)
{
    if (ctx.flags.hasAny(this->getFlags()))
    {
        if (ctx.flags.has(MessageElementFlag::EmoteImages))
        {
            auto images = this->getLoadedImages(container.getImageScale());
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
                auto textCtx = ctx;
                textCtx.flags = MessageElementFlag::Misc;
                this->textElement_->addToContainer(container, textCtx);
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

QJsonObject LayeredEmoteElement::toJson() const
{
    auto base = MessageElement::toJson();
    base["type"_L1] = u"LayeredEmoteElement"_s;

    QJsonArray emotes;
    for (const auto &emote : this->emotes_)
    {
        emotes.append({{
            {"flags"_L1, qmagicenum::enumFlagsName(emote.flags.value())},
            {"emote"_L1, emote.ptr->toJson()},
        }});
    }
    base["emotes"_L1] = emotes;

    QJsonArray tooltips;
    for (const auto &tooltip : this->emoteTooltips_)
    {
        emotes.append(tooltip);
    }
    base["tooltips"_L1] = tooltips;

    if (this->textElement_)
    {
        base["text"_L1] = this->textElement_->toJson();
    }

    base["textElementColor"_L1] = this->textElementColor_.toString();

    return base;
}

// BADGE
BadgeElement::BadgeElement(const EmotePtr &emote, MessageElementFlags flags)
    : MessageElement(flags)
    , emote_(emote)
{
    this->setTooltip(emote->tooltip.string);
}

void BadgeElement::addToContainer(MessageLayoutContainer &container,
                                  const MessageLayoutContext &ctx)
{
    if (ctx.flags.hasAny(this->getFlags()))
    {
        auto image =
            this->emote_->images.getImageOrLoaded(container.getImageScale());
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

QJsonObject BadgeElement::toJson() const
{
    auto base = MessageElement::toJson();
    base["type"_L1] = u"BadgeElement"_s;
    base["emote"_L1] = this->emote_->toJson();

    return base;
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

QJsonObject ModBadgeElement::toJson() const
{
    auto base = BadgeElement::toJson();
    base["type"_L1] = u"ModBadgeElement"_s;

    return base;
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

QJsonObject VipBadgeElement::toJson() const
{
    auto base = BadgeElement::toJson();
    base["type"_L1] = u"VipBadgeElement"_s;

    return base;
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

QJsonObject FfzBadgeElement::toJson() const
{
    auto base = BadgeElement::toJson();
    base["type"_L1] = u"FfzBadgeElement"_s;
    base["color"_L1] = this->color.name(QColor::HexArgb);

    return base;
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
                                 const MessageLayoutContext &ctx)
{
    auto *app = getApp();

    if (ctx.flags.hasAny(this->getFlags()))
    {
        QFontMetrics metrics =
            app->getFonts()->getFontMetrics(this->style_, container.getScale());

        for (const auto &word : this->words_)
        {
            auto wordId = container.nextWordId();

            auto getTextLayoutElement = [&](QString text, int width,
                                            bool hasTrailingSpace) {
                auto color = this->color_.getColor(ctx.messageColors);
                app->getThemes()->normalizeColor(color);

                auto *e = new TextLayoutElement(
                    *this, text, QSize(width, metrics.height()), color,
                    this->style_, container.getScale());
                e->setTrailingSpace(hasTrailingSpace);
                e->setText(text);
                e->setWordId(wordId);

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

const MessageColor &TextElement::color() const noexcept
{
    return this->color_;
}

FontStyle TextElement::fontStyle() const noexcept
{
    return this->style_;
}

void TextElement::appendText(QStringView text)
{
#if QT_VERSION < QT_VERSION_CHECK(6, 0, 0)
    for (auto word : text.split(' '))  // creates a QList
#else
    for (auto word : text.tokenize(u' '))
#endif
    {
        this->words_.append(word.toString());
    }
}

void TextElement::appendText(const QString &text)
{
#if QT_VERSION < QT_VERSION_CHECK(6, 0, 0)
    this->appendText(QStringView{text});
#else
    qsizetype firstSpace = text.indexOf(u' ');
    if (firstSpace == -1)
    {
        // reuse (ref) `text`
        this->words_.emplace_back(text);
        return;
    }

    this->words_.emplace_back(text.sliced(0, firstSpace));
    for (auto word : QStringView{text}.sliced(firstSpace + 1).tokenize(u' '))
    {
        this->words_.emplace_back(word.toString());
    }
#endif
}

QJsonObject TextElement::toJson() const
{
    auto base = MessageElement::toJson();
    base["type"_L1] = u"TextElement"_s;
    base["words"_L1] = QJsonArray::fromStringList(this->words_);
    base["color"_L1] = this->color_.toString();
    base["style"_L1] = qmagicenum::enumNameString(this->style_);

    return base;
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
                                           const MessageLayoutContext &ctx)
{
    auto *app = getApp();

    if (ctx.flags.hasAny(this->getFlags()))
    {
        QFontMetrics metrics =
            app->getFonts()->getFontMetrics(this->style_, container.getScale());

        auto getTextLayoutElement = [&](QString text, int width,
                                        bool hasTrailingSpace) {
            auto color = this->color_.getColor(ctx.messageColors);
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

QJsonObject SingleLineTextElement::toJson() const
{
    auto base = MessageElement::toJson();
    base["type"_L1] = u"SingleLineTextElement"_s;
    QJsonArray words;
    for (const auto &word : this->words_)
    {
        words.append(word.text);
    }
    base["words"_L1] = words;
    base["color"_L1] = this->color_.toString();
    base["style"_L1] = qmagicenum::enumNameString(this->style_);

    return base;
}

LinkElement::LinkElement(const Parsed &parsed, const QString &fullUrl,
                         MessageElementFlags flags, const MessageColor &color,
                         FontStyle style)
    : TextElement({}, flags, color, style)
    , linkInfo_(fullUrl)
    , lowercase_({parsed.lowercase})
    , original_({parsed.original})
{
    this->setTooltip(parsed.original);
}

void LinkElement::addToContainer(MessageLayoutContainer &container,
                                 const MessageLayoutContext &ctx)
{
    this->words_ =
        getSettings()->lowercaseDomains ? this->lowercase_ : this->original_;
    TextElement::addToContainer(container, ctx);
}

Link LinkElement::getLink() const
{
    return {Link::Url, this->linkInfo_.url()};
}

QJsonObject LinkElement::toJson() const
{
    auto base = TextElement::toJson();
    base["type"_L1] = u"LinkElement"_s;
    base["link"_L1] = this->linkInfo_.originalUrl();
    base["lowercase"_L1] = QJsonArray::fromStringList(this->lowercase_);
    base["original"_L1] = QJsonArray::fromStringList(this->original_);

    return base;
}

MentionElement::MentionElement(const QString &displayName, QString loginName_,
                               MessageColor fallbackColor_,
                               MessageColor userColor_)
    : TextElement(displayName,
                  {MessageElementFlag::Text, MessageElementFlag::Mention})
    , fallbackColor(fallbackColor_)
    , userColor(userColor_)
    , userLoginName(std::move(loginName_))
{
}

template <typename>
MentionElement::MentionElement(const QString &displayName, QString loginName_,
                               MessageColor fallbackColor_, QColor userColor_)
    : TextElement(displayName,
                  {MessageElementFlag::Text, MessageElementFlag::Mention})
    , fallbackColor(fallbackColor_)
    , userColor(userColor_.isValid() ? userColor_ : fallbackColor_)
    , userLoginName(std::move(loginName_))
{
}

template MentionElement::MentionElement(const QString &displayName,
                                        QString loginName_,
                                        MessageColor fallbackColor_,
                                        QColor userColor_);

void MentionElement::addToContainer(MessageLayoutContainer &container,
                                    const MessageLayoutContext &ctx)
{
    if (getSettings()->colorUsernames)
    {
        this->color_ = this->userColor;
    }
    else
    {
        this->color_ = this->fallbackColor;
    }

    if (getSettings()->boldUsernames)
    {
        this->style_ = FontStyle::ChatMediumBold;
    }
    else
    {
        this->style_ = FontStyle::ChatMedium;
    }

    TextElement::addToContainer(container, ctx);
}

MessageElement *MentionElement::setLink(const Link &link)
{
    assert(false && "MentionElement::setLink should not be called. Pass "
                    "through a valid login name in the constructor and it will "
                    "automatically be a UserInfo link");

    return TextElement::setLink(link);
}

Link MentionElement::getLink() const
{
    if (this->userLoginName.isEmpty())
    {
        // Some rare mention elements don't have the knowledge of the login name
        return {};
    }

    return {Link::UserInfo, this->userLoginName};
}

QJsonObject MentionElement::toJson() const
{
    auto base = TextElement::toJson();
    base["type"_L1] = u"MentionElement"_s;
    base["fallbackColor"_L1] = this->fallbackColor.toString();
    base["userColor"_L1] = this->userColor.toString();
    base["userLoginName"_L1] = this->userLoginName;

    return base;
}

// TIMESTAMP
TimestampElement::TimestampElement()
    : TimestampElement(getApp()->isTest() ? QTime::fromMSecsSinceStartOfDay(0)
                                          : QTime::currentTime())
{
}

TimestampElement::TimestampElement(QTime time)
    : MessageElement(MessageElementFlag::Timestamp)
    , time_(time)
    , element_(this->formatTime(time))
{
    assert(this->element_ != nullptr);
}

void TimestampElement::addToContainer(MessageLayoutContainer &container,
                                      const MessageLayoutContext &ctx)
{
    if (ctx.flags.hasAny(this->getFlags()))
    {
        if (getSettings()->timestampFormat != this->format_)
        {
            this->format_ = getSettings()->timestampFormat.getValue();
            this->element_.reset(this->formatTime(this->time_));
        }

        this->element_->addToContainer(container, ctx);
    }
}

TextElement *TimestampElement::formatTime(const QTime &time)
{
    static QLocale locale("en_US");

    QString format = locale.toString(time, getSettings()->timestampFormat);

    return new TextElement(format, MessageElementFlag::Timestamp,
                           MessageColor::System, FontStyle::ChatMedium);
}

QJsonObject TimestampElement::toJson() const
{
    auto base = MessageElement::toJson();
    base["type"_L1] = u"TimestampElement"_s;
    base["time"_L1] = this->time_.toString(Qt::ISODate);
    base["element"_L1] = this->element_->toJson();
    base["format"_L1] = this->format_;

    return base;
}

// TWITCH MODERATION
TwitchModerationElement::TwitchModerationElement()
    : MessageElement(MessageElementFlag::ModeratorTools)
{
}

void TwitchModerationElement::addToContainer(MessageLayoutContainer &container,
                                             const MessageLayoutContext &ctx)
{
    if (ctx.flags.has(MessageElementFlag::ModeratorTools))
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

QJsonObject TwitchModerationElement::toJson() const
{
    auto base = MessageElement::toJson();
    base["type"_L1] = u"TwitchModerationElement"_s;

    return base;
}

LinebreakElement::LinebreakElement(MessageElementFlags flags)
    : MessageElement(flags)
{
}

void LinebreakElement::addToContainer(MessageLayoutContainer &container,
                                      const MessageLayoutContext &ctx)
{
    if (ctx.flags.hasAny(this->getFlags()))
    {
        container.breakLine();
    }
}

QJsonObject LinebreakElement::toJson() const
{
    auto base = MessageElement::toJson();
    base["type"_L1] = u"LinebreakElement"_s;

    return base;
}

ScalingImageElement::ScalingImageElement(ImageSet images,
                                         MessageElementFlags flags)
    : MessageElement(flags)
    , images_(std::move(images))
{
}

void ScalingImageElement::addToContainer(MessageLayoutContainer &container,
                                         const MessageLayoutContext &ctx)
{
    if (ctx.flags.hasAny(this->getFlags()))
    {
        const auto &image =
            this->images_.getImageOrLoaded(container.getImageScale());
        if (image->isEmpty())
        {
            return;
        }

        auto size = QSize(image->width() * container.getScale(),
                          image->height() * container.getScale());

        container.addElement(new ImageLayoutElement(*this, image, size));
    }
}

QJsonObject ScalingImageElement::toJson() const
{
    auto base = MessageElement::toJson();
    base["type"_L1] = u"ScalingImageElement"_s;
    base["image"_L1] = this->images_.getImage1()->url().string;

    return base;
}

ReplyCurveElement::ReplyCurveElement()
    : MessageElement(MessageElementFlag::RepliedMessage)
{
}

void ReplyCurveElement::addToContainer(MessageLayoutContainer &container,
                                       const MessageLayoutContext &ctx)
{
    static const int width = 18;         // Overall width
    static const float thickness = 1.5;  // Pen width
    static const int radius = 6;         // Radius of the top left corner
    static const int margin = 2;         // Top/Left/Bottom margin

    if (ctx.flags.hasAny(this->getFlags()))
    {
        float scale = container.getScale();
        container.addElement(
            new ReplyCurveLayoutElement(*this, width * scale, thickness * scale,
                                        radius * scale, margin * scale));
    }
}

QJsonObject ReplyCurveElement::toJson() const
{
    auto base = MessageElement::toJson();
    base["type"_L1] = u"ReplyCurveElement"_s;

    return base;
}

}  // namespace chatterino
