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

    void action() override;

    void paint(QPainter *painter, const QRect &rect) const override;
    QSize sizeHint(const QRect &rect) const override;

private:
    SplitContainer *container_{};
    Split *split_{};
};

}  // namespace chatterino
