#include "messages/MessageElement.hpp"

#include "Application.hpp"
#include "common/Emotemap.hpp"
#include "controllers/moderationactions/ModerationActions.hpp"
#include "debug/Benchmark.hpp"
#include "messages/layouts/MessageLayoutContainer.hpp"
#include "messages/layouts/MessageLayoutElement.hpp"
#include "singletons/Settings.hpp"

namespace chatterino {

MessageElement::MessageElement(Flags flags)
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

const Link &MessageElement::getLink() const
{
    return this->link_;
}

bool MessageElement::hasTrailingSpace() const
{
    return this->trailingSpace;
}

MessageElement::Flags MessageElement::getFlags() const
{
    return this->flags_;
}

// IMAGE
ImageElement::ImageElement(Image *image, MessageElement::Flags flags)
    : MessageElement(flags)
    , image_(image)
{
    this->setTooltip(image->getTooltip());
}

void ImageElement::addToContainer(MessageLayoutContainer &container, MessageElement::Flags flags)
{
    if (flags & this->getFlags()) {
        QSize size(this->image_->getScaledWidth() * container.getScale(),
                   this->image_->getScaledHeight() * container.getScale());

        container.addElement(
            (new ImageLayoutElement(*this, this->image_, size))->setLink(this->getLink()));
    }
}

// EMOTE
EmoteElement::EmoteElement(const EmoteData &data, MessageElement::Flags flags)
    : MessageElement(flags)
    , data(data)
{
    if (data.isValid()) {
        this->setTooltip(data.image1x->getTooltip());
        this->textElement_.reset(
            new TextElement(data.image1x->getCopyString(), MessageElement::Misc));
    }
}

void EmoteElement::addToContainer(MessageLayoutContainer &container, MessageElement::Flags flags)
{
    if (flags & this->getFlags()) {
        if (flags & MessageElement::EmoteImages) {
            if (!this->data.isValid()) {
                return;
            }

            Image *image = this->data.getImage(container.getScale());

            QSize size(int(container.getScale() * image->getScaledWidth()),
                       int(container.getScale() * image->getScaledHeight()));

            container.addElement(
                (new ImageLayoutElement(*this, image, size))->setLink(this->getLink()));
        } else {
            if (this->textElement_) {
                this->textElement_->addToContainer(container, MessageElement::Misc);
            }
        }
    }
}

// TEXT
TextElement::TextElement(const QString &text, MessageElement::Flags flags,
                         const MessageColor &color, FontStyle style)
    : MessageElement(flags)
    , color_(color)
    , style_(style)
{
    for (QString word : text.split(' ')) {
        this->words_.push_back({word, -1});
        // fourtf: add logic to store multiple spaces after message
    }
}

void TextElement::addToContainer(MessageLayoutContainer &container, MessageElement::Flags flags)
{
    auto app = getApp();

    if (flags & this->getFlags()) {
        QFontMetrics metrics = app->fonts->getFontMetrics(this->style_, container.getScale());

        for (Word &word : this->words_) {
            auto getTextLayoutElement = [&](QString text, int width, bool trailingSpace) {
                QColor color = this->color_.getColor(*app->themes);
                app->themes->normalizeColor(color);

                auto e = (new TextLayoutElement(*this, text, QSize(width, metrics.height()), color,
                                                this->style_, container.getScale()))
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
TimestampElement::TimestampElement(QTime time)
    : MessageElement(MessageElement::Timestamp)
    , time_(time)
    , element_(this->formatTime(time))
{
    assert(this->element_ != nullptr);
}

void TimestampElement::addToContainer(MessageLayoutContainer &container,
                                      MessageElement::Flags flags)
{
    if (flags & this->getFlags()) {
        auto app = getApp();
        if (app->settings->timestampFormat != this->format_) {
            this->format_ = app->settings->timestampFormat.getValue();
            this->element_.reset(this->formatTime(this->time_));
        }

        this->element_->addToContainer(container, flags);
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
                                             MessageElement::Flags flags)
{
    if (flags & MessageElement::ModeratorTools) {
        QSize size(int(container.getScale() * 16), int(container.getScale() * 16));

        for (const ModerationAction &m : getApp()->moderationActions->items.getVector()) {
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
