#include "messages/layouts/MessageLayoutElement.hpp"

#include "Application.hpp"
#include "messages/Emote.hpp"
#include "messages/Image.hpp"
#include "messages/layouts/MessageLayoutContext.hpp"
#include "messages/MessageElement.hpp"
#include "providers/twitch/TwitchEmotes.hpp"
#include "util/DebugCount.hpp"

#include <QDebug>
#include <QPainter>
#include <QPainterPath>

namespace {

const QChar RTL_EMBED(0x202B);

void alignRectBottomCenter(QRectF &rect, const QRectF &reference)
{
    QPointF newCenter(reference.center().x(),
                      reference.bottom() - (rect.height() / 2.0));
    rect.moveCenter(newCenter);
}

}  // namespace

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

size_t MessageLayoutElement::getLine() const
{
    return this->line_;
}

void MessageLayoutElement::setLine(size_t line)
{
    this->line_ = line;
}

MessageLayoutElement *MessageLayoutElement::setTrailingSpace(bool value)
{
    this->trailingSpace = value;

    return this;
}

MessageLayoutElement *MessageLayoutElement::setLink(const Link &link)
{
    this->link_ = link;
    return this;
}

MessageLayoutElement *MessageLayoutElement::setText(const QString &_text)
{
    this->text_ = _text;
    return this;
}

Link MessageLayoutElement::getLink() const
{
    if (this->link_)
    {
        return *this->link_;
    }
    return this->creator_.getLink();
}

const QString &MessageLayoutElement::getText() const
{
    return this->text_;
}

FlagsEnum<MessageElementFlag> MessageLayoutElement::getFlags() const
{
    return this->creator_.getFlags();
}

int MessageLayoutElement::getWordId() const
{
    return this->wordId_;
}

void MessageLayoutElement::setWordId(int wordId)
{
    this->wordId_ = wordId;
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

void ImageLayoutElement::addCopyTextToString(QString &str, uint32_t from,
                                             uint32_t to) const
{
    const auto *emoteElement =
        dynamic_cast<EmoteElement *>(&this->getCreator());
    if (emoteElement)
    {
        str += emoteElement->getEmote()->getCopyString();
        str = TwitchEmotes::cleanUpEmoteCode(str);
        if (this->hasTrailingSpace() && to >= 2)
        {
            str += ' ';
        }
    }
}

size_t ImageLayoutElement::getSelectionIndexCount() const
{
    return this->trailingSpace ? 2 : 1;
}

void ImageLayoutElement::paint(QPainter &painter,
                               const MessageColors & /*messageColors*/)
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

bool ImageLayoutElement::paintAnimated(QPainter &painter, int yOffset)
{
    if (this->image_ == nullptr)
    {
        return false;
    }

    if (this->image_->animated())
    {
        if (auto pixmap = this->image_->pixmapOrLoad())
        {
            auto rect = this->getRect();
            rect.moveTop(rect.y() + yOffset);
            painter.drawPixmap(QRectF(rect), *pixmap, QRectF());
            return true;
        }
    }
    return false;
}

int ImageLayoutElement::getMouseOverIndex(const QPoint &abs) const
{
    return 0;
}

int ImageLayoutElement::getXFromIndex(size_t index)
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
// LAYERED IMAGE
//

LayeredImageLayoutElement::LayeredImageLayoutElement(
    MessageElement &creator, std::vector<ImagePtr> images,
    std::vector<QSize> sizes, QSize largestSize)
    : MessageLayoutElement(creator, largestSize)
    , images_(std::move(images))
    , sizes_(std::move(sizes))
{
    assert(this->images_.size() == this->sizes_.size());
    this->trailingSpace = creator.hasTrailingSpace();
}

void LayeredImageLayoutElement::addCopyTextToString(QString &str, uint32_t from,
                                                    uint32_t to) const
{
    const auto *layeredEmoteElement =
        dynamic_cast<LayeredEmoteElement *>(&this->getCreator());
    if (layeredEmoteElement)
    {
        // cleaning is taken care in call
        str += layeredEmoteElement->getCleanCopyString();
        if (this->hasTrailingSpace() && to >= 2)
        {
            str += ' ';
        }
    }
}

size_t LayeredImageLayoutElement::getSelectionIndexCount() const
{
    return this->trailingSpace ? 2 : 1;
}

void LayeredImageLayoutElement::paint(QPainter &painter,
                                      const MessageColors & /*messageColors*/)
{
    auto fullRect = QRectF(this->getRect());

    for (size_t i = 0; i < this->images_.size(); ++i)
    {
        auto &img = this->images_[i];
        if (img == nullptr)
        {
            continue;
        }

        auto pixmap = img->pixmapOrLoad();
        if (img->animated())
        {
            // As soon as we see an animated emote layer, we can stop rendering
            // the static emotes. The paintAnimated function will render any
            // static emotes layered on top of the first seen animated emote.
            return;
        }

        if (pixmap)
        {
            // Matching the web chat behavior, we center the emote within the overall
            // binding box. E.g. small overlay emotes like cvMask will sit in the direct
            // center of even wide emotes.
            auto &size = this->sizes_[i];
            QRectF destRect(0, 0, size.width(), size.height());
            alignRectBottomCenter(destRect, fullRect);

            painter.drawPixmap(destRect, *pixmap, QRectF());
        }
    }
}

bool LayeredImageLayoutElement::paintAnimated(QPainter &painter, int yOffset)
{
    auto fullRect = QRectF(this->getRect());
    fullRect.moveTop(fullRect.y() + yOffset);
    bool animatedFlag = false;

    for (size_t i = 0; i < this->images_.size(); ++i)
    {
        auto &img = this->images_[i];
        if (img == nullptr)
        {
            continue;
        }

        // If we have a static emote layered on top of an animated emote, we need
        // to render the static emote again after animating anything below it.
        if (img->animated() || animatedFlag)
        {
            if (auto pixmap = img->pixmapOrLoad())
            {
                // Matching the web chat behavior, we center the emote within the overall
                // binding box. E.g. small overlay emotes like cvMask will sit in the direct
                // center of even wide emotes.
                auto &size = this->sizes_[i];
                QRectF destRect(0, 0, size.width(), size.height());
                alignRectBottomCenter(destRect, fullRect);

                painter.drawPixmap(destRect, *pixmap, QRectF());
                animatedFlag = true;
            }
        }
    }
    return animatedFlag;
}

int LayeredImageLayoutElement::getMouseOverIndex(const QPoint &abs) const
{
    return 0;
}

int LayeredImageLayoutElement::getXFromIndex(size_t index)
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

void ImageWithBackgroundLayoutElement::paint(
    QPainter &painter, const MessageColors & /*messageColors*/)
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

//
// IMAGE WITH CIRCLE BACKGROUND
//
ImageWithCircleBackgroundLayoutElement::ImageWithCircleBackgroundLayoutElement(
    MessageElement &creator, ImagePtr image, const QSize &imageSize,
    QColor color, int padding)
    : ImageLayoutElement(creator, image,
                         imageSize + QSize(padding, padding) * 2)
    , color_(color)
    , imageSize_(imageSize)
    , padding_(padding)
{
}

void ImageWithCircleBackgroundLayoutElement::paint(
    QPainter &painter, const MessageColors & /*messageColors*/)
{
    if (this->image_ == nullptr)
    {
        return;
    }

    auto pixmap = this->image_->pixmapOrLoad();
    if (pixmap && !this->image_->animated())
    {
        QRectF boxRect(this->getRect());
        painter.setRenderHint(QPainter::Antialiasing);
        painter.setPen(Qt::NoPen);
        painter.setBrush(QBrush(this->color_, Qt::SolidPattern));
        painter.drawEllipse(boxRect);

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

void TextLayoutElement::addCopyTextToString(QString &str, uint32_t from,
                                            uint32_t to) const
{
    str += this->getText().mid(from, to - from);

    if (this->hasTrailingSpace() && to > this->getText().length())
    {
        str += ' ';
    }
}

size_t TextLayoutElement::getSelectionIndexCount() const
{
    return this->getText().length() + (this->trailingSpace ? 1 : 0);
}

void TextLayoutElement::paint(QPainter &painter,
                              const MessageColors & /*messageColors*/)
{
    auto *app = getApp();
    QString text = this->getText();
    if (text.isRightToLeft() || this->reversedNeutral)
    {
        text.prepend(RTL_EMBED);
    }

    painter.setPen(this->color_);

    painter.setFont(app->getFonts()->getFont(this->style_, this->scale_));

    painter.drawText(
        QRectF(this->getRect().x(), this->getRect().y(), 10000, 10000), text,
        QTextOption(Qt::AlignLeft | Qt::AlignTop));
}

bool TextLayoutElement::paintAnimated(QPainter & /*painter*/, int /*yOffset*/)
{
    return false;
}

int TextLayoutElement::getMouseOverIndex(const QPoint &abs) const
{
    if (abs.x() < this->getRect().left())
    {
        return 0;
    }

    auto *app = getApp();

    auto metrics = app->getFonts()->getFontMetrics(this->style_, this->scale_);
    auto x = this->getRect().left();

    for (auto i = 0; i < this->getText().size(); i++)
    {
        auto &&text = this->getText();
        auto width = metrics.horizontalAdvance(this->getText()[i]);

        // accept mouse to be at only 50%+ of character width to increase index
        if (x + (width * 0.5) > abs.x())
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

int TextLayoutElement::getXFromIndex(size_t index)
{
    auto *app = getApp();

    QFontMetrics metrics =
        app->getFonts()->getFontMetrics(this->style_, this->scale_);

    if (index <= 0)
    {
        return this->getRect().left();
    }
    else if (index < static_cast<size_t>(this->getText().size()))
    {
        int x = 0;
        for (size_t i = 0; i < index; i++)
        {
            x += metrics.horizontalAdvance(
                this->getText()[static_cast<QString::size_type>(i)]);
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

void TextIconLayoutElement::addCopyTextToString(QString &str, uint32_t from,
                                                uint32_t to) const
{
}

size_t TextIconLayoutElement::getSelectionIndexCount() const
{
    return this->trailingSpace ? 2 : 1;
}

void TextIconLayoutElement::paint(QPainter &painter,
                                  const MessageColors &messageColors)
{
    auto *app = getApp();

    QFont font = app->getFonts()->getFont(FontStyle::Tiny, this->scale);

    painter.setPen(messageColors.systemText);
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

bool TextIconLayoutElement::paintAnimated(QPainter & /*painter*/,
                                          int /*yOffset*/)
{
    return false;
}

int TextIconLayoutElement::getMouseOverIndex(const QPoint &abs) const
{
    return 0;
}

int TextIconLayoutElement::getXFromIndex(size_t index)
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
                                                 int width, float thickness,
                                                 float radius,
                                                 float neededMargin)
    : MessageLayoutElement(creator, QSize(width, 0))
    , pen_(QColor("#888"), thickness, Qt::SolidLine, Qt::RoundCap)
    , radius_(radius)
    , neededMargin_(neededMargin)
{
}

void ReplyCurveLayoutElement::paint(QPainter &painter,
                                    const MessageColors & /*messageColors*/)
{
    QRectF paintRect(this->getRect());
    QPainterPath path;

    QRectF curveRect = paintRect.marginsRemoved(QMarginsF(
        this->neededMargin_, this->neededMargin_, 0, this->neededMargin_));

    // Make sure that our curveRect can always fit the radius curve
    if (curveRect.height() < this->radius_)
    {
        curveRect.setTop(curveRect.top() -
                         (this->radius_ - curveRect.height()));
    }

    QPointF bStartPoint(curveRect.left(), curveRect.top() + this->radius_);
    QPointF bEndPoint(curveRect.left() + this->radius_, curveRect.top());
    QPointF bControlPoint(curveRect.topLeft());

    // Draw line from bottom left to curve
    path.moveTo(curveRect.bottomLeft());
    path.lineTo(bStartPoint);

    // Draw curve path
    path.quadTo(bControlPoint, bEndPoint);

    // Draw line from curve to top right
    path.lineTo(curveRect.topRight());

    // Render curve
    painter.setPen(this->pen_);
    painter.setRenderHint(QPainter::Antialiasing);
    painter.drawPath(path);
}

bool ReplyCurveLayoutElement::paintAnimated(QPainter & /*painter*/,
                                            int /*yOffset*/)
{
    return false;
}

int ReplyCurveLayoutElement::getMouseOverIndex(const QPoint &abs) const
{
    return 0;
}

int ReplyCurveLayoutElement::getXFromIndex(size_t index)
{
    if (index <= 0)
    {
        return this->getRect().left();
    }

    return this->getRect().right();
}

void ReplyCurveLayoutElement::addCopyTextToString(QString &str, uint32_t from,
                                                  uint32_t to) const
{
}

size_t ReplyCurveLayoutElement::getSelectionIndexCount() const
{
    return 1;
}

}  // namespace chatterino
