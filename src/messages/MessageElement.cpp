#include "messages/MessageElement.hpp"

#include "Application.hpp"
#include "debug/Benchmark.hpp"
#include "messages/Emote.hpp"
#include "messages/layouts/MessageLayoutContainer.hpp"
#include "messages/layouts/MessageLayoutElement.hpp"
#include "singletons/Settings.hpp"
#include "singletons/Theme.hpp"
#include "util/DebugCount.hpp"

namespace chatterino {

static auto IRC_COLORS = [] {
    static QMap<int, QColor> x;
    if (x.isEmpty())
    {
        // Colors taken from https://modern.ircdocs.horse/formatting.html
        x.insert(0, QColor("white"));
        x.insert(1, QColor("black"));
        x.insert(2, QColor("blue"));
        x.insert(3, QColor("green"));
        x.insert(4, QColor("red"));
        x.insert(5, QColor("brown"));
        x.insert(6, QColor("purple"));
        x.insert(7, QColor("orange"));
        x.insert(8, QColor("yellow"));
        x.insert(9, QColor("lightgreen"));
        x.insert(10, QColor("cyan"));
        x.insert(11, QColor("lightcyan"));
        x.insert(12, QColor("lightblue"));
        x.insert(13, QColor("pink"));
        x.insert(14, QColor("gray"));
        x.insert(15, QColor("lightgray"));
        x.insert(16, QColor("#470000"));
        x.insert(17, QColor("#472100"));
        x.insert(18, QColor("#474700"));
        x.insert(19, QColor("#324700"));
        x.insert(20, QColor("#004700"));
        x.insert(21, QColor("#00472c"));
        x.insert(22, QColor("#004747"));
        x.insert(23, QColor("#002747"));
        x.insert(24, QColor("#000047"));
        x.insert(25, QColor("#2e0047"));
        x.insert(26, QColor("#470047"));
        x.insert(27, QColor("#47002a"));
        x.insert(28, QColor("#740000"));
        x.insert(29, QColor("#743a00"));
        x.insert(30, QColor("#747400"));
        x.insert(31, QColor("#517400"));
        x.insert(32, QColor("#007400"));
        x.insert(33, QColor("#007449"));
        x.insert(34, QColor("#007474"));
        x.insert(35, QColor("#004074"));
        x.insert(36, QColor("#000074"));
        x.insert(37, QColor("#4b0074"));
        x.insert(38, QColor("#740074"));
        x.insert(39, QColor("#740045"));
        x.insert(40, QColor("#b50000"));
        x.insert(41, QColor("#b56300"));
        x.insert(42, QColor("#b5b500"));
        x.insert(43, QColor("#7db500"));
        x.insert(44, QColor("#00b500"));
        x.insert(45, QColor("#00b571"));
        x.insert(46, QColor("#00b5b5"));
        x.insert(47, QColor("#0063b5"));
        x.insert(48, QColor("#0000b5"));
        x.insert(49, QColor("#7500b5"));
        x.insert(50, QColor("#b500b5"));
        x.insert(51, QColor("#b5006b"));
        x.insert(52, QColor("#ff0000"));
        x.insert(53, QColor("#ff8c00"));
        x.insert(54, QColor("#ffff00"));
        x.insert(55, QColor("#b2ff00"));
        x.insert(56, QColor("#00ff00"));
        x.insert(57, QColor("#00ffa0"));
        x.insert(58, QColor("#00ffff"));
        x.insert(59, QColor("#008cff"));
        x.insert(60, QColor("#0000ff"));
        x.insert(61, QColor("#a500ff"));
        x.insert(62, QColor("#ff00ff"));
        x.insert(63, QColor("#ff0098"));
        x.insert(64, QColor("#ff5959"));
        x.insert(65, QColor("#ffb459"));
        x.insert(66, QColor("#ffff71"));
        x.insert(67, QColor("#cfff60"));
        x.insert(68, QColor("#6fff6f"));
        x.insert(69, QColor("#65ffc9"));
        x.insert(70, QColor("#6dffff"));
        x.insert(71, QColor("#59b4ff"));
        x.insert(72, QColor("#5959ff"));
        x.insert(73, QColor("#c459ff"));
        x.insert(74, QColor("#ff66ff"));
        x.insert(75, QColor("#ff59bc"));
        x.insert(76, QColor("#ff9c9c"));
        x.insert(77, QColor("#ffd39c"));
        x.insert(78, QColor("#ffff9c"));
        x.insert(79, QColor("#e2ff9c"));
        x.insert(80, QColor("#9cff9c"));
        x.insert(81, QColor("#9cffdb"));
        x.insert(82, QColor("#9cffff"));
        x.insert(83, QColor("#9cd3ff"));
        x.insert(84, QColor("#9c9cff"));
        x.insert(85, QColor("#dc9cff"));
        x.insert(86, QColor("#ff9cff"));
        x.insert(87, QColor("#ff94d3"));
        x.insert(88, QColor("#000000"));
        x.insert(89, QColor("#131313"));
        x.insert(90, QColor("#282828"));
        x.insert(91, QColor("#363636"));
        x.insert(92, QColor("#4d4d4d"));
        x.insert(93, QColor("#656565"));
        x.insert(94, QColor("#818181"));
        x.insert(95, QColor("#9f9f9f"));
        x.insert(96, QColor("#bcbcbc"));
        x.insert(97, QColor("#e2e2e2"));
        x.insert(98, QColor("#ffffff"));
    }
    return x;
}();

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

static QRegularExpression regex("\u0003(\\d{1,2})?(,(\\d{1,2}))?",
                                QRegularExpression::UseUnicodePropertiesOption);

// TEXT
IrcTextElement::IrcTextElement(const QString &fullText,
                               MessageElementFlags flags, FontStyle style)
    : MessageElement(flags)
    , style_(style)
{
    assert(regex.isValid());

    int fg = -1, bg = -1;

    for (const auto &text : fullText.split(' '))
    {
        std::vector<Segment> segments;

        int pos = 0;
        int lastPos = 0;

        auto i = regex.globalMatch(text);

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
                qDebug() << "Push segment with text" << seg.text;
            }

            if (!match.captured(1).isEmpty())
            {
                fg = match.captured(1).toInt(nullptr);
                qDebug() << "Set paint brush FG to" << fg;
            }
            else
            {
                fg = -1;
                qDebug() << "Set paint brush FG to" << fg;
            }
            if (!match.captured(3).isEmpty())
            {
                bg = match.captured(3).toInt(nullptr);
                qDebug() << "Set paint brush BG to" << bg;
            }
            else if (fg == -1)
            {
                bg = -1;
                qDebug() << "Set paint brush BG to" << bg;
            }

            lastPos = match.capturedStart() + match.capturedLength();
        }

        auto seg = Segment{};
        seg.text = text.mid(lastPos);
        seg.fg = fg;
        seg.bg = bg;
        segments.emplace_back(seg);
        qDebug() << "Push segment with text 2" << seg.text;

        QString n(text);

        n.replace(regex, "");

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

        int fg = -1;
        int bg = -1;

        for (auto &word : this->words_)
        {
            auto getTextLayoutElement = [&](QString text,
                                            std::vector<Segment> segments,
                                            int width, bool hasTrailingSpace) {
                std::vector<PajSegment> xd{};

                for (const auto &segment : segments)
                {
                    QColor color = defaultColor;
                    if (segment.fg != -1)
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
                        text.mid(wordStart, i - wordStart), {}, width, false));
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
                text.mid(wordStart), {}, width, this->hasTrailingSpace()));
            container.breakLine();
        }
    }
}

}  // namespace chatterino
