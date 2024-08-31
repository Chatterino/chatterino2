#include "widgets/splits/InputCompletionItem.hpp"

#include "messages/Emote.hpp"
#include "messages/Image.hpp"

namespace chatterino {

InputCompletionItem::InputCompletionItem(const EmotePtr &emote,
                                         const QString &text,
                                         ActionCallback action)
    : emote_(emote)
    , text_(text)
    , action_(action)
{
}

void InputCompletionItem::action()
{
    if (this->action_)
    {
        if (this->emote_)
        {
            this->action_(this->emote_->name.string);
        }
        else
        {
            this->action_(this->text_);
        }
    }
}

void InputCompletionItem::paint(QPainter *painter, const QRect &rect) const
{
    auto margin = 4;
    QRect textRect;
    if (this->emote_)
    {
        painter->setRenderHint(QPainter::SmoothPixmapTransform);
        painter->setRenderHint(QPainter::Antialiasing);

        auto imageHeight = ICON_SIZE.height() - margin * 2;

        QRect iconRect{
            rect.topLeft() + QPoint{margin, margin},
            QSize{imageHeight, imageHeight},
        };

        if (auto image = this->emote_->images.getImage(2))
        {
            if (auto pixmap = image->pixmapOrLoad())
            {
                if (image->height() != 0)
                {
                    auto aspectRatio =
                        double(image->width()) / double(image->height());

                    iconRect = {
                        rect.topLeft() + QPoint{margin, margin},
                        QSize(int(imageHeight * aspectRatio), imageHeight)};
                    painter->drawPixmap(iconRect, *pixmap);
                }
            }
        }

        textRect =
            QRect(iconRect.topRight() + QPoint{margin, 0},
                  QSize(rect.width() - iconRect.width(), iconRect.height()));
    }
    else
    {
        textRect = QRect(rect.topLeft() + QPoint{margin, 0},
                         QSize(rect.width(), rect.height()));
    }

    painter->drawText(textRect, Qt::AlignLeft | Qt::AlignVCenter, this->text_);
}

QSize InputCompletionItem::sizeHint(const QRect &rect) const
{
    return QSize(rect.width(), ICON_SIZE.height());
}

}  // namespace chatterino
