#include "messages/layouts/MessageLayoutElement.hpp"

#include "Application.hpp"
#include "messages/Emote.hpp"
#include "messages/Image.hpp"
#include "messages/MessageElement.hpp"
#include "util/DebugCount.hpp"

#include <QDebug>
#include <QPainter>

namespace chatterino
{
    const QRect& MessageLayoutElement::getRect() const
    {
        return this->rect_;
    }

    MessageLayoutElement::MessageLayoutElement(
        MessageElement& creator, const QSize& size)
        : creator_(creator)
    {
        this->rect_.setSize(size);
        DebugCount::increase("message layout elements");
    }

    MessageLayoutElement::~MessageLayoutElement()
    {
        DebugCount::decrease("message layout elements");
    }

    MessageElement& MessageLayoutElement::getCreator() const
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

    MessageLayoutElement* MessageLayoutElement::setTrailingSpace(bool value)
    {
        this->trailingSpace = value;

        return this;
    }

    MessageLayoutElement* MessageLayoutElement::setLink(const Link& _link)
    {
        this->link_ = _link;
        return this;
    }

    MessageLayoutElement* MessageLayoutElement::setText(const QString& _text)
    {
        this->text_ = _text;
        return this;
    }

    const Link& MessageLayoutElement::getLink() const
    {
        return this->link_;
    }

    const QString& MessageLayoutElement::getText() const
    {
        return this->text_;
    }

    FlagsEnum<MessageElementFlag> MessageLayoutElement::getFlags() const
    {
        return this->creator_.getFlags();
    }

    //
    // IMAGE
    //

    ImageLayoutElement::ImageLayoutElement(
        MessageElement& creator, ImagePtr image, const QSize& size)
        : MessageLayoutElement(creator, size)
        , image_(image)
    {
        this->trailingSpace = creator.hasTrailingSpace();
    }

    void ImageLayoutElement::addCopyTextToString(
        QString& str, int from, int to) const
    {
        const auto* emoteElement =
            dynamic_cast<EmoteElement*>(&this->getCreator());
        if (emoteElement)
        {
            str += emoteElement->getEmote()->getCopyString();
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

    void ImageLayoutElement::paint(QPainter& painter)
    {
        if (this->image_ == nullptr)
        {
            return;
        }

        auto pixmap = this->image_->pixmap();
        if (pixmap && !this->image_->animated())
        {
            // fourtf: make it use qreal values
            painter.drawPixmap(QRectF(this->getRect()), *pixmap, QRectF());
        }
    }

    void ImageLayoutElement::paintAnimated(QPainter& painter, int yOffset)
    {
        if (this->image_ == nullptr)
        {
            return;
        }

        if (this->image_->animated())
        {
            if (auto pixmap = this->image_->pixmap())
            {
                auto rect = this->getRect();
                rect.moveTop(rect.y() + yOffset);
                painter.drawPixmap(QRectF(rect), *pixmap, QRectF());
            }
        }
    }

    int ImageLayoutElement::getMouseOverIndex(const QPoint& abs) const
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
    // TEXT
    //

    TextLayoutElement::TextLayoutElement(MessageElement& _creator,
        QString& _text, const QSize& _size, QBrush brush, FontStyle _style,
        float _scale)
        : MessageLayoutElement(_creator, _size)
        , brush_(brush)
        , style_(_style)
        , scale_(_scale)
    {
        this->setText(_text);
    }

    TextLayoutElement::TextLayoutElement(MessageElement& _creator, QFont font,
        QString& _text, const QSize& _size, QBrush brush, FontStyle _style,
        float _scale)
        : MessageLayoutElement(_creator, _size)
        , brush_(brush)
        , style_(_style)
        , font_(font)
        , scale_(_scale)
    {
        this->setText(_text);
    }

    void TextLayoutElement::addCopyTextToString(
        QString& str, int from, int to) const
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

    void TextLayoutElement::paint(QPainter& painter)
    {
        painter.setPen(QPen(this->brush_, 1));

        painter.setFont(this->font_);

        // painter.setFont(getFonts()->getFont(this->style_, this->scale_));

        painter.drawText(
            QRectF(this->getRect().x(), this->getRect().y(), 10000, 10000),
            this->getText(), QTextOption(Qt::AlignLeft | Qt::AlignTop));
    }

    void TextLayoutElement::paintAnimated(QPainter&, int)
    {
    }

    int TextLayoutElement::getMouseOverIndex(const QPoint& abs) const
    {
        if (abs.x() < this->getRect().left())
        {
            return 0;
        }

        auto metrics = getFonts()->getFontMetrics(this->style_, this->scale_);
        auto x = this->getRect().left();

        for (auto i = 0; i < this->getText().size(); i++)
        {
            auto&& text = this->getText();
            auto width = metrics.width(this->getText()[i]);

            if (x + width > abs.x())
            {
                if (text.size() > i + 1 &&
                    QChar::isLowSurrogate(text[i].unicode()))
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

        return this->getSelectionIndexCount() -
               (this->hasTrailingSpace() ? 1 : 0);
    }

    int TextLayoutElement::getXFromIndex(int index)
    {
        auto metrics = getFonts()->getFontMetrics(this->style_, this->scale_);

        if (index <= 0)
        {
            return this->getRect().left();
        }
        else if (index < this->getText().size())
        {
            int x = 0;
            for (int i = 0; i < index; i++)
            {
                x += metrics.width(this->getText()[i]);
            }
            return x + this->getRect().left();
        }
        else
        {
            return this->getRect().right();
        }
    }

    // TEXT ICON
    TextIconLayoutElement::TextIconLayoutElement(MessageElement& creator,
        const QString& _line1, const QString& _line2, float _scale,
        const QSize& size)
        : MessageLayoutElement(creator, size)
        , scale(_scale)
        , line1(_line1)
        , line2(_line2)
    {
    }

    void TextIconLayoutElement::addCopyTextToString(
        QString& str, int from, int to) const
    {
    }

    int TextIconLayoutElement::getSelectionIndexCount() const
    {
        return this->trailingSpace ? 2 : 1;
    }

    void TextIconLayoutElement::paint(QPainter& painter)
    {
        QFont font = getFonts()->getFont(FontStyle::Tiny, this->scale);

        // painter.setPen(app->themes->messages.textColors.system);
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
            painter.drawText(
                QPoint(this->getRect().x(),
                    this->getRect().y() + this->getRect().height()),
                this->line2);
        }
    }

    void TextIconLayoutElement::paintAnimated(QPainter& painter, int yOffset)
    {
    }

    int TextIconLayoutElement::getMouseOverIndex(const QPoint& abs) const
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

    // TestLayoutElement
    TestLayoutElement::TestLayoutElement(MessageElement& element,
        const QSize& size, const QColor& background, bool end)
        : MessageLayoutElement(element, size)
        , size_(size)
        , background_(background)
        , end_(end)
    {
    }

    void TestLayoutElement::addCopyTextToString(
        QString& str, int from, int to) const
    {
    }

    int TestLayoutElement::getSelectionIndexCount() const
    {
        return 0;
    }

    void TestLayoutElement::paint(QPainter& painter)
    {
        const auto dy = this->getRect().y();
        const auto color = end_ ? background_ : QColor(0, 0, 0, 127);

        // make zig zag
        auto polygon = QPolygon();
        for (auto x = size_.height() / -2; x < size_.width() + 16;
             x += size_.height())
        {
            polygon.push_back({x, dy + 0});
            polygon.push_back({x + size_.height(), dy + size_.height()});
            x += size_.height();
            polygon.push_back({x, dy + size_.height()});
            polygon.push_back({x + size_.height(), dy + 0});
        }

        // finish polygon
        polygon.push_back({size_.width(), 1000});
        polygon.push_back({0, 1000});

        // finish polygon
        polygon.push_back({size_.width(), 1000});
        polygon.push_back({0, 1000});

        // turn into path
        auto path = QPainterPath();
        path.addPolygon(polygon);

        // draw
        painter.fillPath(path, color);
        painter.strokePath(path, QColor(127, 127, 127, 127));
    }

    void TestLayoutElement::paintAnimated(QPainter& painter, int yOffset)
    {
    }

    int TestLayoutElement::getMouseOverIndex(const QPoint& abs) const
    {
        return 0;
    }

    int TestLayoutElement::getXFromIndex(int index)
    {
        return 0;
    }

}  // namespace chatterino
