#include "widgets/dialogs/switcher/NewTabItem.hpp"

#include "Application.hpp"
#include "providers/twitch/TwitchIrcServer.hpp"
#include "singletons/WindowManager.hpp"
#include "widgets/Notebook.hpp"
#include "widgets/Window.hpp"
#include "widgets/helper/NotebookTab.hpp"
#include "widgets/splits/Split.hpp"

namespace chatterino {

NewTabItem::NewTabItem(const QString &text, const QString &channelName)
    : SwitcherItem(QIcon(":/switcher/plus.svg"), text)
    , channelName_(channelName)
{
}

void NewTabItem::action()
{
    auto &nb = getApp()->windows->getMainWindow().getNotebook();
    SplitContainer *container = nb.addPage(true);

    Split *split = new Split(container);
    split->setChannel(
        getApp()->twitch.server->getOrAddChannel(this->channelName_));
    container->appendSplit(split);
}

}  // namespace chatterino
