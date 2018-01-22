#pragma once
#include "widgets/basewindow.hpp"

#include <QLabel>
#include <QWidget>

namespace chatterino {
namespace widgets {

class TooltipWidget : public BaseWindow
{
    Q_OBJECT
public:
    TooltipWidget(BaseWidget *parent = nullptr);

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

private:
    QLabel *displayText;

    void moveIntoDesktopRect(QWidget *parent);
};

}  // namespace widgets
}  // namespace chatterino
