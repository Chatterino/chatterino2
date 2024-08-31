#include "widgets/dialogs/switcher/NewTabItem.hpp"

#include "Application.hpp"
#include "providers/twitch/TwitchIrcServer.hpp"
#include "singletons/Fonts.hpp"
#include "singletons/Theme.hpp"
#include "singletons/WindowManager.hpp"
#include "widgets/helper/NotebookTab.hpp"
#include "widgets/Notebook.hpp"
#include "widgets/splits/Split.hpp"
#include "widgets/Window.hpp"

namespace chatterino {

NewTabItem::NewTabItem(Window *window_, const QString &channelName)
    : AbstractSwitcherItem(QIcon(":/switcher/plus.svg"))
    , channelName_(channelName)
    , text_(QString(TEXT_FORMAT).arg(channelName))
    , window(window_)
{
}

void NewTabItem::action()
{
    auto &nb = this->window->getNotebook();
    SplitContainer *container = nb.addPage(true);

    Split *split = new Split(container);
    split->setChannel(
        getApp()->getTwitch()->getOrAddChannel(this->channelName_));
    container->insertSplit(split);
}

void NewTabItem::paint(QPainter *painter, const QRect &rect) const
{
    painter->save();

    painter->setRenderHint(QPainter::Antialiasing, true);

    // TODO(leon): Right pen/brush/font settings?
    painter->setPen(getApp()->getThemes()->splits.header.text);
    painter->setBrush(Qt::SolidPattern);
    painter->setFont(
        getApp()->getFonts()->getFont(FontStyle::UiMediumBold, 1.0));

    QRect iconRect(rect.topLeft(), ICON_SIZE);
    this->icon_.paint(painter, iconRect, Qt::AlignLeft | Qt::AlignVCenter);

    QRect textRect =
        QRect(iconRect.topRight(),
              QSize(rect.width() - iconRect.width(), iconRect.height()));
    painter->drawText(textRect, Qt::AlignLeft | Qt::AlignVCenter, this->text_);

    painter->restore();
}

QSize NewTabItem::sizeHint(const QRect &rect) const
{
    return QSize(rect.width(), ICON_SIZE.height());
}

}  // namespace chatterino
