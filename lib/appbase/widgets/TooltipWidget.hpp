#pragma once

#include "widgets/BaseWindow.hpp"

#include <QLabel>
#include <QWidget>
#include <pajlada/signals/signal.hpp>

namespace AB_NAMESPACE {

class TooltipWidget : public BaseWindow
{
    Q_OBJECT

public:
    static TooltipWidget *getInstance();

    TooltipWidget(BaseWidget *parent = nullptr);
    ~TooltipWidget() override;

    void setText(QString text);
    void setWordWrap(bool wrap);
    void clearImage();
    void setImage(QPixmap image);

#ifdef USEWINSDK
    void raise();
#endif

protected:
    void changeEvent(QEvent *) override;
    void leaveEvent(QEvent *) override;
    void themeChangedEvent() override;
    void scaleChangedEvent(float) override;
    void paintEvent(QPaintEvent *) override;

private:
    void updateFont();

    QLabel *displayImage_;
    QLabel *displayText_;
    pajlada::Signals::Connection fontChangedConnection_;
};

}  // namespace AB_NAMESPACE
