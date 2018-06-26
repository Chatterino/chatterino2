#pragma once

#include "widgets/basewindow.hpp"

#include <QLabel>
#include <QWidget>
#include <pajlada/signals/signal.hpp>

namespace chatterino {
namespace widgets {

class TooltipWidget : public BaseWindow
{
    Q_OBJECT

public:
    static TooltipWidget *getInstance();

    TooltipWidget(BaseWidget *parent = nullptr);
    virtual ~TooltipWidget() override;

    void setText(QString text);

#ifdef USEWINSDK
    void raise();
#endif

protected:
    void changeEvent(QEvent *) override;
    void leaveEvent(QEvent *) override;
    void themeRefreshEvent() override;
    void scaleChangedEvent(float) override;

private:
    QLabel *displayText;
    pajlada::Signals::Connection fontChangedConnection;

    void updateFont();
};

}  // namespace widgets
}  // namespace chatterino
