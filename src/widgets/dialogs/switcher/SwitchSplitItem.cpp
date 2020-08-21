#include "widgets/dialogs/switcher/SwitchSplitItem.hpp"

#include "Application.hpp"
#include "singletons/Fonts.hpp"
#include "singletons/Theme.hpp"
#include "widgets/helper/NotebookTab.hpp"

namespace chatterino {

SwitchSplitItem::SwitchSplitItem(Split *split)
    : AbstractSwitcherItem(QIcon(":switcher/switch.svg"))
    , split_(split)
    , container_(split->getContainer())
{
}

SwitchSplitItem::SwitchSplitItem(SplitContainer *container)
    : AbstractSwitcherItem(QIcon(":switcher/switch.svg"))
    , container_(container)
{
}

void SwitchSplitItem::action()
{
    auto &nb = getApp()->windows->getMainWindow().getNotebook();
    nb.select(this->container_);

    /*
     * If the item is referring to a specific channel, select the
     * corresponding split.
     */
    if (this->split_)
    {
        this->container_->setSelected(this->split_);
    }
}

void SwitchSplitItem::paint(QPainter *painter, const QRect &rect) const
{
    painter->save();

    painter->setRenderHint(QPainter::Antialiasing, true);

    // TODO(leon): Right pen/brush/font settings?
    painter->setPen(getApp()->themes->splits.header.text);
    painter->setBrush(Qt::SolidPattern);
    painter->setFont(getApp()->fonts->getFont(FontStyle::UiMediumBold, 1.0));

    QRect iconRect(rect.topLeft(), ICON_SIZE);
    this->icon_.paint(painter, iconRect, Qt::AlignLeft | Qt::AlignVCenter);

    if (this->split_)
    {
        // Draw channel name and name of the containing tab
        const auto availableTextWidth = rect.width() - iconRect.width();
        QRect leftTextRect =
            QRect(iconRect.topRight(),
                  QSize(0.3 * availableTextWidth, iconRect.height()));

        painter->drawText(leftTextRect, Qt::AlignLeft | Qt::AlignVCenter,
                          this->split_->getChannel()->getName());

        QRect rightTextRect =
            QRect(leftTextRect.topRight(),
                  QSize(0.7 * availableTextWidth, iconRect.height()));

        painter->setFont(getApp()->fonts->getFont(FontStyle::UiMedium, 1.0));
        painter->drawText(rightTextRect, Qt::AlignRight | Qt::AlignVCenter,
                          this->container_->getTab()->getTitle());
    }
    else if (!this->split_ && this->container_)
    {
        // Only draw name of tab
        QRect textRect =
            QRect(iconRect.topRight(),
                  QSize(rect.width() - iconRect.width(), iconRect.height()));

        painter->drawText(textRect, Qt::AlignLeft | Qt::AlignVCenter,
                          this->container_->getTab()->getTitle());
    }

    painter->restore();
}

QSize SwitchSplitItem::sizeHint(const QRect &rect) const
{
    return QSize(rect.width(), ICON_SIZE.height());
}

}  // namespace chatterino
