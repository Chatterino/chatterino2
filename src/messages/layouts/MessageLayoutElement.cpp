#include "messages/layouts/MessageLayoutElement.hpp"

#include "Application.hpp"
#include "messages/Emote.hpp"
#include "messages/Image.hpp"
#include "messages/MessageElement.hpp"
#include "providers/twitch/TwitchEmotes.hpp"
#include "singletons/Theme.hpp"
#include "util/DebugCount.hpp"

#include <QDebug>
#include <QPainter>
#include <QPainterPath>

namespace chatterino {

const QRect &MessageLayoutElement::getRect() const
{
    return this->rect_;
}

MessageLayoutElement::MessageLayoutElement(MessageElement &creator,
                                           const QSize &size)
    : creator_(creator)
{
    this->rect_.setSize(size);
    DebugCount::increase("message layout elements");
}

MessageLayoutElement::~MessageLayoutElement()
{
    DebugCount::decrease("message layout elements");
}

MessageElement &MessageLayoutElement::getCreator() const
{
    return this->creator_;
}

void MessageLayoutElement::setPosition(QPoint point)
{
    this->rect_.moveTopLeft(point);
}

bool MessageLayoutElement::hasTrailingSpace() const
{
    return this->trailingSpace;
}

int MessageLayoutElement::getLine() const
{
    return this->line_;
}

void MessageLayoutElement::setLine(int line)
{
    this->line_ = line;
}

MessageLayoutElement *MessageLayoutElement::setTrailingSpace(bool value)
{
    this->trailingSpace = value;

    return this;
}

MessageLayoutElement *MessageLayoutElement::setLink(const Link &_link)
{
    this->link_ = _link;
    return this;
}

MessageLayoutElement *MessageLayoutElement::setText(const QString &_text)
{
    this->text_ = _text;
    return this;
}

const Link &MessageLayoutElement::getLink() const
{
    return this->link_;
}

const QString &MessageLayoutElement::getText() const
{
    return this->text_;
}

FlagsEnum<MessageElementFlag> MessageLayoutElement::getFlags() const
{
    return this->creator_.getFlags();
}

//
// Floating Layout Element
//

const QRect &FloatingMessageLayoutElement::getRect() const
{
    return this->rect_;
}

FloatingMessageLayoutElement::FloatingMessageLayoutElement(
    MessageElement &creator, const QSize &size)
    : creator_(creator)
{
    this->rect_.setSize(size);
    DebugCount::increase("message layout elements");
}

FloatingMessageLayoutElement::~FloatingMessageLayoutElement()
{
    DebugCount::decrease("message layout elements");
}

MessageElement &FloatingMessageLayoutElement::getCreator() const
{
    return this->creator_;
}

FloatingMessageLayoutElement *FloatingMessageLayoutElement::setLink(
    const Link &_link)
{
    this->link_ = _link;
    return this;
}

const Link &FloatingMessageLayoutElement::getLink() const
{
    return this->link_;
}

//
// IMAGE
//

ImageLayoutElement::ImageLayoutElement(MessageElement &creator, ImagePtr image,
                                       const QSize &size)
    : MessageLayoutElement(creator, size)
    , image_(std::move(image))
{
    this->trailingSpace = creator.hasTrailingSpace();
}

void ImageLayoutElement::addCopyTextToString(QString &str, int from,
                                             int to) const
{
    const auto *emoteElement =
        dynamic_cast<EmoteElement *>(&this->getCreator());
    if (emoteElement)
    {
        str += emoteElement->getEmote()->getCopyString();
        str = TwitchEmotes::cleanUpEmoteCode(str);
        if (this->hasTrailingSpace())
        {
            str += " ";
        }
    }
}

int ImageLayoutElement::getSelectionIndexCount() const
{
    return this->trailingSpace ? 2 : 1;
}

void ImageLayoutElement::paint(QPainter &painter)
{
    if (this->image_ == nullptr)
    {
        return;
    }

    auto pixmap = this->image_->pixmapOrLoad();
    if (pixmap && !this->image_->animated())
    {
        // fourtf: make it use qreal values
        painter.drawPixmap(QRectF(this->getRect()), *pixmap, QRectF());
    }
}

void ImageLayoutElement::paintAnimated(QPainter &painter, int yOffset)
{
    if (this->image_ == nullptr)
    {
        return;
    }

    if (this->image_->animated())
    {
        if (auto pixmap = this->image_->pixmapOrLoad())
        {
            auto rect = this->getRect();
            rect.moveTop(rect.y() + yOffset);
            painter.drawPixmap(QRectF(rect), *pixmap, QRectF());
        }
    }
}

int ImageLayoutElement::getMouseOverIndex(const QPoint &abs) const
{
    return 0;
}

int ImageLayoutElement::getXFromIndex(int index)
{
    if (index <= 0)
    {
        return this->getRect().left();
    }
    else if (index == 1)
    {
        // fourtf: remove space width
        return this->getRect().right();
    }
    else
    {
        return this->getRect().right();
    }
}

//
// IMAGE WITH BACKGROUND
//
ImageWithBackgroundLayoutElement::ImageWithBackgroundLayoutElement(
    MessageElement &creator, ImagePtr image, const QSize &size, QColor color)
    : ImageLayoutElement(creator, image, size)
    , color_(color)
{
}

void ImageWithBackgroundLayoutElement::paint(QPainter &painter)
{
    if (this->image_ == nullptr)
    {
        return;
    }

    auto pixmap = this->image_->pixmapOrLoad();
    if (pixmap && !this->image_->animated())
    {
        painter.fillRect(QRectF(this->getRect()), this->color_);

        // fourtf: make it use qreal values
        painter.drawPixmap(QRectF(this->getRect()), *pixmap, QRectF());
    }
}

PrettyFloatingImageLayoutElement::PrettyFloatingImageLayoutElement(
    MessageElement &creator, ImagePtr image, const QSize &size, int padding,
    QColor background)
    : FloatingMessageLayoutElement(creator, size + QSize(padding, padding) * 2)
    , image_(std::move(image))
    , padding_(padding)
    , background_(background)
    , imageSize_(size)
{
}

void PrettyFloatingImageLayoutElement::paint(QPainter &painter)
{
    if (this->image_ == nullptr)
    {
        return;
    }

    auto pixmap = this->image_->pixmapOrLoad();
    if (pixmap && !this->image_->animated())
    {
        QRectF boxRect(this->getRect());
        QPainterPath path;
        path.addRoundedRect(boxRect, this->padding_, this->padding_);
        painter.fillPath(path, this->background_);

        QRectF imgRect;
        imgRect.setTopLeft(boxRect.topLeft());
        imgRect.setSize(this->imageSize_);
        imgRect.translate(this->padding_, this->padding_);

        painter.drawPixmap(imgRect, *pixmap, QRectF());
    }
}

//
// TEXT
//

TextLayoutElement::TextLayoutElement(MessageElement &_creator, QString &_text,
                                     const QSize &_size, QColor _color,
                                     FontStyle _style, float _scale)
    : MessageLayoutElement(_creator, _size)
    , color_(_color)
    , style_(_style)
    , scale_(_scale)
{
    this->setText(_text);
}

void TextLayoutElement::listenToLinkChanges()
{
    this->managedConnections_.managedConnect(
        static_cast<TextElement &>(this->getCreator()).linkChanged, [this]() {
            this->setLink(this->getCreator().getLink());
        });
}

void TextLayoutElement::addCopyTextToString(QString &str, int from,
                                            int to) const
{
    str += this->getText().mid(from, to - from);

    if (this->hasTrailingSpace())
    {
        str += " ";
    }
}

int TextLayoutElement::getSelectionIndexCount() const
{
    return this->getText().length() + (this->trailingSpace ? 1 : 0);
}

void TextLayoutElement::paint(QPainter &painter)
{
    auto app = getApp();

    painter.setPen(this->color_);

    painter.setFont(app->fonts->getFont(this->style_, this->scale_));

    painter.drawText(
        QRectF(this->getRect().x(), this->getRect().y(), 10000, 10000),
        this->getText(), QTextOption(Qt::AlignLeft | Qt::AlignTop));
}

void TextLayoutElement::paintAnimated(QPainter &, int)
{
}

int TextLayoutElement::getMouseOverIndex(const QPoint &abs) const
{
    if (abs.x() < this->getRect().left())
    {
        return 0;
    }

    auto app = getApp();

    auto metrics = app->fonts->getFontMetrics(this->style_, this->scale_);
    auto x = this->getRect().left();

    for (auto i = 0; i < this->getText().size(); i++)
    {
        auto &&text = this->getText();
        auto width = metrics.horizontalAdvance(this->getText()[i]);

        if (x + width > abs.x())
        {
            if (text.size() > i + 1 && QChar::isLowSurrogate(text[i].unicode()))
            {
                i++;
            }

            return i;
        }

        x += width;
    }

    //    if (this->hasTrailingSpace() && abs.x() < this->getRect().right())
    //    {
    //        return this->getSelectionIndexCount() - 1;
    //    }

    return this->getSelectionIndexCount() - (this->hasTrailingSpace() ? 1 : 0);
}

int TextLayoutElement::getXFromIndex(int index)
{
    auto app = getApp();

    QFontMetrics metrics =
        app->fonts->getFontMetrics(this->style_, this->scale_);

    if (index <= 0)
    {
        return this->getRect().left();
    }
    else if (index < this->getText().size())
    {
        int x = 0;
        for (int i = 0; i < index; i++)
        {
            x += metrics.horizontalAdvance(this->getText()[i]);
        }
        return x + this->getRect().left();
    }
    else
    {
        return this->getRect().right();
    }
}

// TEXT ICON
TextIconLayoutElement::TextIconLayoutElement(MessageElement &creator,
                                             const QString &_line1,
                                             const QString &_line2,
                                             float _scale, const QSize &size)
    : MessageLayoutElement(creator, size)
    , scale(_scale)
    , line1(_line1)
    , line2(_line2)
{
}

void TextIconLayoutElement::addCopyTextToString(QString &str, int from,
                                                int to) const
{
}

int TextIconLayoutElement::getSelectionIndexCount() const
{
    return this->trailingSpace ? 2 : 1;
}

void TextIconLayoutElement::paint(QPainter &painter)
{
    auto app = getApp();

    QFont font = app->fonts->getFont(FontStyle::Tiny, this->scale);

    painter.setPen(app->themes->messages.textColors.system);
    painter.setFont(font);

    QTextOption option;
    option.setAlignment(Qt::AlignHCenter);

    if (this->line2.isEmpty())
    {
        QRect _rect(this->getRect());
        painter.drawText(_rect, this->line1, option);
    }
    else
    {
        painter.drawText(
            QPoint(this->getRect().x(),
                   this->getRect().y() + this->getRect().height() / 2),
            this->line1);
        painter.drawText(QPoint(this->getRect().x(),
                                this->getRect().y() + this->getRect().height()),
                         this->line2);
    }
}

void TextIconLayoutElement::paintAnimated(QPainter &painter, int yOffset)
{
}

int TextIconLayoutElement::getMouseOverIndex(const QPoint &abs) const
{
    return 0;
}

int TextIconLayoutElement::getXFromIndex(int index)
{
    if (index <= 0)
    {
        return this->getRect().left();
    }
    else if (index == 1)
    {
        // fourtf: remove space width
        return this->getRect().right();
    }
    else
    {
        return this->getRect().right();
    }
}

ReplyCurveLayoutElement::ReplyCurveLayoutElement(MessageElement &creator,
                                                 const QSize &size,
                                                 float thickness,
                                                 float neededMargin)
    : MessageLayoutElement(creator, size)
    , pen_(QColor("#888"), thickness, Qt::SolidLine, Qt::RoundCap)
    , neededMargin_(neededMargin)
{
}

void ReplyCurveLayoutElement::paint(QPainter &painter)
{
    QRectF paintRect(this->getRect());
    QPainterPath bezierPath;

    qreal top = paintRect.top() + paintRect.height() * 0.25;  // 25% from top
    qreal left = paintRect.left() + this->neededMargin_;
    qreal bottom = paintRect.bottom() - this->neededMargin_;
    QPointF startPoint(left, bottom);
    QPointF controlPoint(left, top);
    QPointF endPoint(paintRect.right(), top);

    // Create curve path
    bezierPath.moveTo(startPoint);
    bezierPath.quadTo(controlPoint, endPoint);

    // Render curve
    painter.setPen(this->pen_);
    painter.setRenderHint(QPainter::Antialiasing);
    painter.drawPath(bezierPath);
}

void ReplyCurveLayoutElement::paintAnimated(QPainter &painter, int yOffset)
{
}

int ReplyCurveLayoutElement::getMouseOverIndex(const QPoint &abs) const
{
    return 0;
}

int ReplyCurveLayoutElement::getXFromIndex(int index)
{
    if (index <= 0)
    {
        return this->getRect().left();
    }
    else if (index == 1)
    {
        // fourtf: remove space width
        return this->getRect().right();
    }
    else
    {
        return this->getRect().right();
    }
}

void ReplyCurveLayoutElement::addCopyTextToString(QString &str, int from,
                                                  int to) const
{
}

int ReplyCurveLayoutElement::getSelectionIndexCount() const
{
    return 1;
}

}  // namespace chatterino
