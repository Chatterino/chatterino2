#pragma once

#include "widgets/dialogs/switcher/AbstractSwitcherItem.hpp"

#include <QPainter>
#include <QRect>
#include <QSize>

namespace chatterino {

class SplitContainer;
class Split;

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
