#pragma once
#include "widgets/basewidget.hpp"

#include <QLabel>
#include <QWidget>

namespace chatterino {
namespace widgets {

class TooltipWidget : public BaseWidget
{
    Q_OBJECT
public:
    TooltipWidget(BaseWidget *parent = nullptr);

    void setText(QString text);
    void moveTo(QPoint point);

    static TooltipWidget *getInstance()
    {
        static TooltipWidget *tooltipWidget = nullptr;
        if (tooltipWidget == nullptr) {
            tooltipWidget = new TooltipWidget();
        }
        return tooltipWidget;
    }

private:
    QLabel *displayText;
};

}  // namespace widgets
}  // namespace chatterino
