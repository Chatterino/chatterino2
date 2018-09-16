#include "TooltipWidget.hpp"

#include "Application.hpp"
#include "singletons/Fonts.hpp"
#include "singletons/Theme.hpp"

#include <QDebug>
#include <QDesktopWidget>
#include <QStyle>
#include <QVBoxLayout>

#ifdef USEWINSDK
#    include <Windows.h>
#endif

namespace chatterino {

TooltipWidget *TooltipWidget::getInstance()
{
    static TooltipWidget *tooltipWidget = nullptr;
    if (tooltipWidget == nullptr) {
        tooltipWidget = new TooltipWidget();
    }
    return tooltipWidget;
}

TooltipWidget::TooltipWidget(BaseWidget *parent)
    : BaseWindow(parent, BaseWindow::TopMost)
    , displayText_(new QLabel())
{
    auto app = getApp();

    this->setStyleSheet("color: #fff; background: #000");
    this->setWindowOpacity(0.8);
    this->updateFont();
    this->setStayInScreenRect(true);

    this->setAttribute(Qt::WA_ShowWithoutActivating);
    this->setWindowFlags(Qt::Tool | Qt::FramelessWindowHint |
                         Qt::X11BypassWindowManagerHint |
                         Qt::BypassWindowManagerHint);

    displayText_->setAlignment(Qt::AlignHCenter);
    displayText_->setText("tooltip text");
    auto layout = new QVBoxLayout();
    layout->setContentsMargins(10, 5, 10, 5);
    layout->addWidget(displayText_);
    this->setLayout(layout);

    this->fontChangedConnection_ =
        app->fonts->fontChanged.connect([this] { this->updateFont(); });
}

TooltipWidget::~TooltipWidget()
{
    this->fontChangedConnection_.disconnect();
}

#ifdef USEWINSDK
void TooltipWidget::raise()
{
    ::SetWindowPos(HWND(this->winId()), HWND_TOPMOST, 0, 0, 0, 0,
                   SWP_NOMOVE | SWP_NOSIZE | SWP_NOACTIVATE);
}
#endif

void TooltipWidget::themeChangedEvent()
{
    this->setStyleSheet("color: #fff; background: #000");
}

void TooltipWidget::scaleChangedEvent(float)
{
    this->updateFont();
}

void TooltipWidget::updateFont()
{
    auto app = getApp();

    this->setFont(
        app->fonts->getFont(FontStyle::ChatMediumSmall, this->getScale()));
}

void TooltipWidget::setText(QString text)
{
    this->displayText_->setText(text);
}

void TooltipWidget::setWordWrap(bool wrap)
{
    this->displayText_->setWordWrap(wrap);
}

void TooltipWidget::changeEvent(QEvent *)
{
    // clear parents event
}

void TooltipWidget::leaveEvent(QEvent *)
{
    // clear parents event
}

}  // namespace chatterino
