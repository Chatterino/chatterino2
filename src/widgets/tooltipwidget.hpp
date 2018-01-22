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
    TooltipWidget(BaseWidget *parent = nullptr);
    ~TooltipWidget();

    void setText(QString text);
    void moveTo(QWidget *widget, QPoint point);

    static TooltipWidget *getInstance()
    {
        static TooltipWidget *tooltipWidget = nullptr;
        if (tooltipWidget == nullptr) {
            tooltipWidget = new TooltipWidget();
        }
        return tooltipWidget;
    }

protected:
    virtual void resizeEvent(QResizeEvent *) override;
    virtual void changeEvent(QEvent *) override;
    virtual void leaveEvent(QEvent *) override;
    virtual void dpiMultiplierChanged(float, float) override;

private:
    QLabel *displayText;
    pajlada::Signals::Connection fontChangedConnection;

    void moveIntoDesktopRect(QWidget *parent);
    void updateFont();
};

}  // namespace widgets
}  // namespace chatterino
