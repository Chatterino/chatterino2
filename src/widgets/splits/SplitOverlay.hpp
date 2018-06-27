#pragma once

#include <QGridLayout>
#include <QPushButton>

#include "pajlada/signals/signalholder.hpp"
#include "widgets/BaseWidget.hpp"

namespace chatterino {

class Split;

class SplitOverlay : public BaseWidget, pajlada::Signals::SignalHolder
{
public:
    explicit SplitOverlay(Split *parent = nullptr);

protected:
    //    bool event(QEvent *event) override;
    void paintEvent(QPaintEvent *event) override;
    void resizeEvent(QResizeEvent *event) override;
    void mouseMoveEvent(QMouseEvent *event) override;

private:
    // fourtf: !!! preserve the order of left, up, right and down
    enum HoveredElement { None, SplitMove, SplitLeft, SplitUp, SplitRight, SplitDown };
    HoveredElement hoveredElement = None;
    Split *split;
    QGridLayout *_layout;
    QPushButton *_left;
    QPushButton *_up;
    QPushButton *_right;
    QPushButton *_down;

    class ButtonEventFilter : public QObject
    {
        SplitOverlay *parent;
        HoveredElement hoveredElement;

    public:
        ButtonEventFilter(SplitOverlay *parent, HoveredElement hoveredElement);

    protected:
        bool eventFilter(QObject *watched, QEvent *event) override;
    };

    friend class ButtonEventFilter;
};

}  // namespace chatterino
