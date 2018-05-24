#include "tooltipwidget.hpp"

#include "application.hpp"
#include "singletons/fontmanager.hpp"
#include "singletons/thememanager.hpp"

#include <QDebug>
#include <QDesktopWidget>
#include <QStyle>
#include <QVBoxLayout>

#ifdef USEWINSDK
#include <Windows.h>
#endif

namespace chatterino {
namespace widgets {

TooltipWidget::TooltipWidget(BaseWidget *parent)
    : BaseWindow(parent, BaseWindow::TopMost)
    , displayText(new QLabel())
{
    auto app = getApp();

    this->setStyleSheet("color: #fff; background: #000");
    this->setWindowOpacity(0.8);
    this->updateFont();
    this->setStayInScreenRect(true);

    this->setAttribute(Qt::WA_ShowWithoutActivating);
    this->setWindowFlags(Qt::FramelessWindowHint | Qt::X11BypassWindowManagerHint |
                         Qt::BypassWindowManagerHint);

    displayText->setAlignment(Qt::AlignHCenter);
    displayText->setText("tooltip text");
    auto layout = new QVBoxLayout();
    layout->setContentsMargins(10, 5, 10, 5);
    layout->addWidget(displayText);
    this->setLayout(layout);

    this->fontChangedConnection = app->fonts->fontChanged.connect([this] { this->updateFont(); });
}

TooltipWidget::~TooltipWidget()
{
    this->fontChangedConnection.disconnect();
}

#ifdef USEWINSDK
void TooltipWidget::raise()
{
    ::SetWindowPos((HWND)this->winId(), HWND_TOPMOST, 0, 0, 0, 0,
                   SWP_NOMOVE | SWP_NOSIZE | SWP_NOACTIVATE);
}
#endif

void TooltipWidget::themeRefreshEvent()
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
        app->fonts->getFont(singletons::FontManager::Type::ChatMediumSmall, this->getScale()));
}

void TooltipWidget::setText(QString text)
{
    this->displayText->setText(text);
}

void TooltipWidget::changeEvent(QEvent *)
{
    // clear parents event
}

void TooltipWidget::leaveEvent(QEvent *)
{
    // clear parents event
}

}  // namespace widgets
}  // namespace chatterino
