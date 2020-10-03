#include "EmoteInputItem.hpp"

namespace chatterino {

EmoteInputItem::EmoteInputItem(const EmotePtr &emote, const QString &text,
                               ActionCallback action)
    : emote_(emote)
    , text_(text)
    , action_(action)
{
}

void EmoteInputItem::action()
{
    if (this->action_ && this->emote_)
        this->action_(this->emote_->name.string);
}

void EmoteInputItem::paint(QPainter *painter, const QRect &rect) const
{
    painter->setRenderHint(QPainter::SmoothPixmapTransform);
    painter->setRenderHint(QPainter::Antialiasing);

    auto margin = 4;
    auto imageHeight = ICON_SIZE.height() - margin * 2;

    QRect iconRect{
        rect.topLeft() + QPoint{margin, margin},
        QSize{imageHeight, imageHeight},
    };

    if (this->emote_)
    {
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
    }

    QRect textRect =
        QRect(iconRect.topRight() + QPoint{margin, 0},
              QSize(rect.width() - iconRect.width(), iconRect.height()));
    painter->drawText(textRect, Qt::AlignLeft | Qt::AlignVCenter, this->text_);
}

QSize EmoteInputItem::sizeHint(const QRect &rect) const
{
    return QSize(rect.width(), ICON_SIZE.height());
}

}  // namespace chatterino
