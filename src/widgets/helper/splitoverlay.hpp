#pragma once

#include "pajlada/signals/signalholder.hpp"
#include "widgets/basewidget.hpp"

namespace chatterino {
namespace widgets {

class Split;

class SplitOverlay : public BaseWidget, pajlada::Signals::SignalHolder
{
public:
    explicit SplitOverlay(Split *parent = nullptr);

protected:
    void paintEvent(QPaintEvent *event) override;

private:
    // fourtf: !!! preserve the order of left, up, right and down
    enum HoveredElement { None, SplitMove, SplitLeft, SplitUp, SplitRight, SplitDown };
    HoveredElement hoveredElement = None;
    Split *split;

    class ButtonEventFilter : public QObject
    {
        HoveredElement hoveredElement;
        SplitOverlay *parent;

    public:
        ButtonEventFilter(SplitOverlay *parent, HoveredElement hoveredElement);

    protected:
        bool eventFilter(QObject *watched, QEvent *event) override;
    };

    friend class ButtonEventFilter;
};

}  // namespace widgets
}  // namespace chatterino
