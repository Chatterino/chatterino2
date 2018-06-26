#include "messages/MessageElement.hpp"

#include "Application.hpp"
#include "messages/layouts/MessageLayoutContainer.hpp"
#include "messages/layouts/MessageLayoutElement.hpp"
#include "singletons/SettingsManager.hpp"
#include "debug/Benchmark.hpp"
#include "common/Emotemap.hpp"

namespace chatterino {

MessageElement::MessageElement(Flags _flags)
    : flags(_flags)
{
    DebugCount::increase("message elements");
}

MessageElement::~MessageElement()
{
    DebugCount::decrease("message elements");
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
ImageElement::ImageElement(Image *_image, MessageElement::Flags flags)
    : MessageElement(flags)
    , image(_image)
{
    this->setTooltip(_image->getTooltip());
}

void ImageElement::addToContainer(MessageLayoutContainer &container, MessageElement::Flags _flags)
{
    if (_flags & this->getFlags()) {
        QSize size(this->image->getScaledWidth() * container.getScale(),
                   this->image->getScaledHeight() * container.getScale());

        container.addElement(
            (new ImageLayoutElement(*this, this->image, size))->setLink(this->getLink()));
    }
}

// EMOTE
EmoteElement::EmoteElement(const EmoteData &_data, MessageElement::Flags flags)
    : MessageElement(flags)
    , data(_data)
{
    if (_data.isValid()) {
        this->setTooltip(data.image1x->getTooltip());
        this->textElement.reset(
            new TextElement(_data.image1x->getCopyString(), MessageElement::Misc));
    }
}

void EmoteElement::addToContainer(MessageLayoutContainer &container, MessageElement::Flags _flags)
{
    if (_flags & this->getFlags()) {
        if (_flags & MessageElement::EmoteImages) {
            if (!this->data.isValid()) {
                return;
            }

            Image *_image = this->data.getImage(container.getScale());

            QSize size(int(container.getScale() * _image->getScaledWidth()),
                       int(container.getScale() * _image->getScaledHeight()));

            container.addElement(
                (new ImageLayoutElement(*this, _image, size))->setLink(this->getLink()));
        } else {
            if (this->textElement) {
                this->textElement->addToContainer(container, MessageElement::Misc);
            }
        }
    }
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
        // fourtf: add logic to store multiple spaces after message
    }
}

void TextElement::addToContainer(MessageLayoutContainer &container, MessageElement::Flags _flags)
{
    auto app = getApp();

    if (_flags & this->getFlags()) {
        QFontMetrics metrics = app->fonts->getFontMetrics(this->style, container.getScale());

        for (Word &word : this->words) {
            auto getTextLayoutElement = [&](QString text, int width, bool trailingSpace) {
                QColor color = this->color.getColor(*app->themes);
                app->themes->normalizeColor(color);

                auto e = (new TextLayoutElement(*this, text, QSize(width, metrics.height()), color,
                                                this->style, container.getScale()))
                             ->setLink(this->getLink());
                e->setTrailingSpace(trailingSpace);
                return e;
            };

            // fourtf: add again
            //            if (word.width == -1) {
            word.width = metrics.width(word.text);
            //            }

            // see if the text fits in the current line
            if (container.fitsInLine(word.width)) {
                container.addElementNoLineBreak(
                    getTextLayoutElement(word.text, word.width, this->hasTrailingSpace()));
                continue;
            }

            // see if the text fits in the next line
            if (!container.atStartOfLine()) {
                container.breakLine();

                if (container.fitsInLine(word.width)) {
                    container.addElementNoLineBreak(
                        getTextLayoutElement(word.text, word.width, this->hasTrailingSpace()));
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
                int charWidth = metrics.width(text[i]);

                if (!container.fitsInLine(width + charWidth)) {
                    container.addElementNoLineBreak(
                        getTextLayoutElement(text.mid(wordStart, i - wordStart), width, false));
                    container.breakLine();

                    wordStart = i;
                    lastWidth = width;
                    width = 0;
                    if (textLength > i + 2) {
                        width += metrics.width(text[i]);
                        width += metrics.width(text[i + 1]);
                        i += 1;
                    }
                    continue;
                }
                width += charWidth;
            }

            UNUSED(lastWidth);  // XXX: What should this be used for (if anything)? KKona

            container.addElement(
                getTextLayoutElement(text.mid(wordStart), width, this->hasTrailingSpace()));
            container.breakLine();
        }
    }
}

// TIMESTAMP
TimestampElement::TimestampElement(QTime _time)
    : MessageElement(MessageElement::Timestamp)
    , time(_time)
    , element(this->formatTime(_time))
{
    assert(this->element != nullptr);
}

void TimestampElement::addToContainer(MessageLayoutContainer &container,
                                      MessageElement::Flags _flags)
{
    if (_flags & this->getFlags()) {
        auto app = getApp();
        if (app->settings->timestampFormat != this->format) {
            this->format = app->settings->timestampFormat.getValue();
            this->element.reset(this->formatTime(this->time));
        }

        this->element->addToContainer(container, _flags);
    }
}

TextElement *TimestampElement::formatTime(const QTime &time)
{
    static QLocale locale("en_US");

    QString format = locale.toString(time, getApp()->settings->timestampFormat);

    return new TextElement(format, Flags::Timestamp, MessageColor::System, FontStyle::ChatMedium);
}

// TWITCH MODERATION
TwitchModerationElement::TwitchModerationElement()
    : MessageElement(MessageElement::ModeratorTools)
{
}

void TwitchModerationElement::addToContainer(MessageLayoutContainer &container,
                                             MessageElement::Flags _flags)
{
    if (_flags & MessageElement::ModeratorTools) {
        QSize size((int)(container.getScale() * 16), (int)(container.getScale() * 16));

        for (const chatterino::ModerationAction &m : getApp()->settings->getModerationActions()) {
            if (m.isImage()) {
                container.addElement((new ImageLayoutElement(*this, m.getImage(), size))
                                         ->setLink(Link(Link::UserAction, m.getAction())));
            } else {
                container.addElement((new TextIconLayoutElement(*this, m.getLine1(), m.getLine2(),
                                                                container.getScale(), size))
                                         ->setLink(Link(Link::UserAction, m.getAction())));
            }
        }
    }
}

}  // namespace chatterino
