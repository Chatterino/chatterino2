#include "widgets/dialogs/switcher/SwitchSplitItem.hpp"

#include "Application.hpp"
#include "widgets/helper/NotebookTab.hpp"

namespace chatterino {

SwitchSplitItem::SwitchSplitItem(const QString &text, Split *split)
    : SwitcherItem(text)
    , split_(split)
{
}

void SwitchSplitItem::action()
{
    auto &nb = getApp()->windows->getMainWindow().getNotebook();
    nb.select(split_->getContainer());
}

}  // namespace chatterino
