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
    if (this->emote_)
    {
        if (auto image = this->emote_->images.getImage(4))
        {
            if (auto pixmap = image->pixmapOrLoad())
            {
                painter->drawPixmap(QRect(rect.x(), rect.y(), ICON_SIZE.width(),
                                          ICON_SIZE.height()),
                                    *pixmap);
            }
        }
    }

    QRect iconRect(rect.topLeft(), ICON_SIZE);
    QRect textRect =
        QRect(iconRect.topRight(),
              QSize(rect.width() - iconRect.width(), iconRect.height()));
    painter->drawText(textRect, Qt::AlignLeft | Qt::AlignVCenter, this->text_);
}

QSize EmoteInputItem::sizeHint(const QRect &rect) const
{
    return QSize(rect.width(), ICON_SIZE.height());
}

}  // namespace chatterino
