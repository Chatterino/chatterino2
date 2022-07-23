#include "widgets/dialogs/switcher/NewPopupItem.hpp"

#include "Application.hpp"
#include "providers/twitch/TwitchIrcServer.hpp"
#include "singletons/Fonts.hpp"
#include "singletons/Theme.hpp"
#include "singletons/WindowManager.hpp"
#include "widgets/Notebook.hpp"
#include "widgets/Window.hpp"
#include "widgets/helper/NotebookTab.hpp"
#include "widgets/splits/Split.hpp"

namespace chatterino {

NewPopupItem::NewPopupItem(const QString &channelName)
    : AbstractSwitcherItem(QIcon(":/switcher/popup.svg"))
    , channelName_(channelName)
    , text_(QString(TEXT_FORMAT).arg(channelName))
{
}

void NewPopupItem::action()
{
    auto *app = getApp();
    auto channel = app->twitch->getOrAddChannel(this->channelName_);
    app->windows->openInPopup(channel);
}

void NewPopupItem::paint(QPainter *painter, const QRect &rect) const
{
    painter->save();

    painter->setRenderHint(QPainter::Antialiasing, true);

    painter->setPen(getApp()->themes->splits.header.text);
    painter->setBrush(Qt::SolidPattern);
    painter->setFont(getApp()->fonts->getFont(FontStyle::UiMediumBold, 1.0));

    QRect iconRect(rect.topLeft(), ICON_SIZE);
    this->icon_.paint(painter, iconRect, Qt::AlignLeft | Qt::AlignVCenter);

    QRect textRect =
        QRect(iconRect.topRight(),
              QSize(rect.width() - iconRect.width(), iconRect.height()));
    painter->drawText(textRect, Qt::AlignLeft | Qt::AlignVCenter, this->text_);

    painter->restore();
}

QSize NewPopupItem::sizeHint(const QRect &rect) const
{
    return QSize(rect.width(), ICON_SIZE.height());
}

}  // namespace chatterino
