#include "messages/messageelement.hpp"
#include "messages/layouts/messagelayoutcontainer.hpp"
#include "messages/layouts/messagelayoutelement.hpp"
#include "singletons/settingsmanager.hpp"
#include "util/benchmark.hpp"
#include "util/emotemap.hpp"

namespace chatterino {
namespace messages {

MessageElement::MessageElement(Flags _flags)
    : flags(_flags)
{
}

MessageElement *MessageElement::setLink(const Link &_link)
{
    this->link = _link;
    return this;
}

MessageElement *MessageElement::setTooltip(const QString &_tooltip)
{
    this->tooltip = _tooltip;
    return this;
}

MessageElement *MessageElement::setTrailingSpace(bool value)
{
    this->trailingSpace = value;
    return this;
}

const QString &MessageElement::getTooltip() const
{
    return this->tooltip;
}

const Link &MessageElement::getLink() const
{
    return this->link;
}

bool MessageElement::hasTrailingSpace() const
{
    return this->trailingSpace;
}

MessageElement::Flags MessageElement::getFlags() const
{
    return this->flags;
}

// IMAGE
ImageElement::ImageElement(Image &_image, MessageElement::Flags flags)
    : MessageElement(flags)
    , image(_image)
{
    this->setTooltip(_image.getTooltip());
}

void ImageElement::addToContainer(MessageLayoutContainer &container, MessageElement::Flags _flags)
{
    QSize size(this->image.getWidth() * this->image.getScale() * container.scale,
               this->image.getHeight() * this->image.getScale() * container.scale);

    container.addElement(new ImageLayoutElement(*this, this->image, size));
}

void ImageElement::update(UpdateFlags _flags)
{
}

// EMOTE
EmoteElement::EmoteElement(const util::EmoteData &_data, MessageElement::Flags flags)
    : MessageElement(flags)
    , data(_data)
{
    if (_data.isValid()) {
        this->setTooltip(data.image1x->getTooltip());
    }
}

void EmoteElement::addToContainer(MessageLayoutContainer &container, MessageElement::Flags _flags)
{
    if (!this->data.isValid()) {
        qDebug() << "EmoteElement::data is invalid xD";
        return;
    }

    int quality = singletons::SettingManager::getInstance().preferredEmoteQuality;

    Image *_image;
    if (quality == 3 && this->data.image3x != nullptr) {
        _image = this->data.image3x;
    } else if (quality >= 2 && this->data.image2x != nullptr) {
        _image = this->data.image2x;
    } else {
        _image = this->data.image1x;
    }

    QSize size((int)(container.scale * _image->getScaledWidth()),
               (int)(container.scale * _image->getScaledHeight()));

    container.addElement(new ImageLayoutElement(*this, *_image, size));
}

void EmoteElement::update(UpdateFlags _flags)
{
}

// TEXT
TextElement::TextElement(const QString &text, MessageElement::Flags flags,
                         const MessageColor &_color, FontStyle _style)
    : MessageElement(flags)
    , color(_color)
    , style(_style)
{
    for (QString word : text.split(' ')) {
        this->words.push_back({word, -1});
        // fourtf: add logic to store mutliple spaces after message
    }
}

void TextElement::addToContainer(MessageLayoutContainer &container, MessageElement::Flags _flags)
{
    QFontMetrics &metrics =
        singletons::FontManager::getInstance().getFontMetrics(this->style, container.scale);
    singletons::ThemeManager &themeManager = singletons::ThemeManager::ThemeManager::getInstance();

    for (Word &word : this->words) {
        auto getTextLayoutElement = [&](QString text, int width) {
            return new TextLayoutElement(*this, text, QSize(width, metrics.height()),
                                         this->color.getColor(themeManager), this->style,
                                         container.scale);
        };

        if (word.width == -1) {
            word.width = metrics.width(word.text);
        }

        // see if the text fits in the current line
        if (container.fitsInLine(word.width)) {
            container.addElementNoLineBreak(getTextLayoutElement(word.text, word.width));
            continue;
        }

        // see if the text fits in the next line
        if (!container.atStartOfLine()) {
            container.breakLine();

            if (container.fitsInLine(word.width)) {
                container.addElementNoLineBreak(getTextLayoutElement(word.text, word.width));
                continue;
            }
        }

        // we done goofed, we need to wrap the text
        QString text = word.text;
        int textLength = text.length();
        int wordStart = 0;
        int width = metrics.width(text[0]);
        int lastWidth = 0;

        for (int i = 1; i < textLength; i++) {
            int chatWidth = metrics.width(text[i]);

            if (!container.fitsInLine(width + chatWidth)) {
                container.addElementNoLineBreak(
                    getTextLayoutElement(text.mid(wordStart, i - wordStart), width - lastWidth));
                container.breakLine();

                i += 2;
                wordStart = i;
                lastWidth = width;
                width += chatWidth;
                continue;
            }
        }

        container.addElement(getTextLayoutElement(text.mid(wordStart), word.width - lastWidth));
    }
}

void TextElement::update(UpdateFlags _flags)
{
    if (_flags & UpdateFlags::Update_Text) {
        for (Word &word : this->words) {
            word.width = -1;
        }
    }
}

// TIMESTAMP
TimestampElement::TimestampElement()
    : TimestampElement(QTime::currentTime())
{
}

TimestampElement::TimestampElement(QTime _time)
    : MessageElement(MessageElement::Timestamp)
    , time(_time)
    , element(formatTime(_time))
{
    assert(this->element != nullptr);
}

TimestampElement::~TimestampElement()
{
    delete this->element;
}

void TimestampElement::addToContainer(MessageLayoutContainer &container,
                                      MessageElement::Flags _flags)
{
    this->element->addToContainer(container, _flags);
}

void TimestampElement::update(UpdateFlags _flags)
{
    if (_flags == UpdateFlags::Update_All) {
        this->element = TimestampElement::formatTime(this->time);
    } else {
        this->element->update(_flags);
    }
}

TextElement *TimestampElement::formatTime(const QTime &time)
{
    QString text = time.toString(singletons::SettingManager::getInstance().timestampFormat);

    return new TextElement(text, Flags::Timestamp, MessageColor::System, FontStyle::Medium);
}

// TWITCH MODERATION
TwitchModerationElement::TwitchModerationElement()
    : MessageElement(MessageElement::ModeratorTools)
{
}

void TwitchModerationElement::addToContainer(MessageLayoutContainer &container,
                                             MessageElement::Flags _flags)
{
}

void TwitchModerationElement::update(UpdateFlags _flags)
{
}
}  // namespace messages
}  // namespace chatterino
