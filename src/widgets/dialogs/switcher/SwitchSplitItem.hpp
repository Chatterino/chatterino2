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
    SwitchSplitItem(Split *split);
    SwitchSplitItem(SplitContainer *container);

    virtual void action() override;

    virtual void paint(QPainter *painter, const QRect &rect) const;
    virtual QSize sizeHint(const QRect &rect) const;

private:
    Split *split_{};
    SplitContainer *container_{};
};

}  // namespace chatterino
