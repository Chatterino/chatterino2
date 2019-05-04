#include "messages/MessageElement.hpp"

#include "Application.hpp"
#include "controllers/moderationactions/ModerationActions.hpp"
#include "debug/Benchmark.hpp"
#include "messages/Emote.hpp"
#include "messages/layouts/MessageLayoutContainer.hpp"
#include "messages/layouts/MessageLayoutElement.hpp"
#include "singletons/Settings.hpp"
#include "singletons/Theme.hpp"
#include "util/DebugCount.hpp"

namespace chatterino {

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
            auto image = this->emote_->images.getImage(container.getScale());
            if (image->isEmpty())
                return;

            auto emoteScale =
                this->getFlags().hasAny(MessageElementFlag::Badges)
                    ? 1
                    : getSettings()->emoteScale.getValue();

            auto size =
                QSize(int(container.getScale() * image->width() * emoteScale),
                      int(container.getScale() * image->height() * emoteScale));

            container.addElement((new ImageLayoutElement(*this, image, size))
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
                                            bool trailingSpace) {
                auto color = this->color_.getColor(*app->themes);
                app->themes->normalizeColor(color);

                auto e = (new TextLayoutElement(
                              *this, text, QSize(width, metrics.height()),
                              color, this->style_, container.getScale()))
                             ->setLink(this->getLink());
                e->setTrailingSpace(trailingSpace);
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

            container.addElement(getTextLayoutElement(
                text.mid(wordStart), width, this->hasTrailingSpace()));
            container.breakLine();
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

        for (const auto &action :
             getApp()->moderationActions->items.getVector())
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

}  // namespace chatterino
