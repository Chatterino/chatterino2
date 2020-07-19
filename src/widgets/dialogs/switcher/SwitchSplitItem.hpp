#pragma once

#include "widgets/dialogs/switcher/AbstractSwitcherItem.hpp"

#include "singletons/WindowManager.hpp"
#include "widgets/Notebook.hpp"
#include "widgets/Window.hpp"
#include "widgets/helper/NotebookTab.hpp"
#include "widgets/splits/Split.hpp"

namespace chatterino {

class SwitchSplitItem : public AbstractSwitcherItem
{
public:
    SwitchSplitItem(const QString &text, Split *split);

    virtual void action() override;

private:
    Split *split_;
};

}  // namespace chatterino
