#include "TooltipWidget.hpp"

#include "Application.hpp"
#include "BaseTheme.hpp"
#include "singletons/Fonts.hpp"
#include "singletons/WindowManager.hpp"
#include "widgets/Window.hpp"

#include <QDebug>
#include <QDesktopWidget>
#include <QStyle>
#include <QVBoxLayout>

#ifdef USEWINSDK
#    include <Windows.h>
#endif

#include <QPainter>

namespace chatterino {

TooltipWidget *TooltipWidget::instance()
{
    auto *mainWindow =
        static_cast<QWidget *>(&(getApp()->windows->getMainWindow()));
    static TooltipWidget *tooltipWidget = new TooltipWidget(mainWindow);
    return tooltipWidget;
}

TooltipWidget::TooltipWidget(QWidget *parent)
    : BaseWindow({BaseWindow::TopMost, BaseWindow::DontFocus}, parent)
    , displayImage_(new QLabel())
    , displayText_(new QLabel())
{
    assert(parent != nullptr);
    this->setStyleSheet("color: #fff; background: rgba(11, 11, 11, 0.8)");
    this->setAttribute(Qt::WA_TranslucentBackground);
    //this->setWindowOpacity(0.8);
    this->updateFont();
    this->setStayInScreenRect(true);

    displayImage_->hide();
    displayImage_->setAlignment(Qt::AlignHCenter);
    displayImage_->setStyleSheet("background: transparent");
    displayText_->setAlignment(Qt::AlignHCenter);
    displayText_->setText("tooltip text");
    displayText_->setStyleSheet("background: transparent");
    auto layout = new QVBoxLayout();
    layout->setContentsMargins(10, 5, 10, 5);
    layout->addWidget(displayImage_);
    layout->addWidget(displayText_);
    this->setLayout(layout);

    this->fontChangedConnection_ = getFonts()->fontChanged.connect([this] {
        this->updateFont();
    });
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
    //    this->setStyleSheet("color: #fff; background: #000");
}

void TooltipWidget::paintEvent(QPaintEvent *)
{
    QPainter painter(this);

    painter.fillRect(this->rect(), QColor(0, 0, 0, int(0.8 * 255)));
}

void TooltipWidget::scaleChangedEvent(float)
{
    this->updateFont();
}

void TooltipWidget::updateFont()
{
    this->setFont(
        getFonts()->getFont(FontStyle::ChatMediumSmall, this->scale()));
}

void TooltipWidget::setText(QString text)
{
    this->displayText_->setText(text);
}

void TooltipWidget::setWordWrap(bool wrap)
{
    this->displayText_->setWordWrap(wrap);
}

void TooltipWidget::clearImage()
{
    this->displayImage_->hide();
}

void TooltipWidget::setImage(QPixmap image)
{
    this->displayImage_->show();
    this->displayImage_->setPixmap(image);
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
