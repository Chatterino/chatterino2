#pragma once

#include "singletons/WindowManager.hpp"
#include "widgets/dialogs/switcher/AbstractSwitcherItem.hpp"
#include "widgets/helper/NotebookTab.hpp"
#include "widgets/Notebook.hpp"
#include "widgets/splits/Split.hpp"
#include "widgets/Window.hpp"

namespace chatterino {

class SwitchSplitItem : public AbstractSwitcherItem
{
public:
    SwitchSplitItem(SplitContainer *container, Split *split = nullptr);

    virtual void action() override;

    virtual void paint(QPainter *painter, const QRect &rect) const override;
    virtual QSize sizeHint(const QRect &rect) const override;

private:
    SplitContainer *container_{};
    Split *split_{};
};

}  // namespace chatterino
