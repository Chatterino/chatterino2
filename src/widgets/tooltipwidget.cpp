#include "tooltipwidget.hpp"
#include "singletons/fontmanager.hpp"
#include "singletons/thememanager.hpp"

#include <QDebug>
#include <QDesktopWidget>
#include <QStyle>
#include <QVBoxLayout>

namespace chatterino {
namespace widgets {

TooltipWidget::TooltipWidget(BaseWidget *parent)
    : BaseWindow(parent)
    , displayText(new QLabel())
{
    this->setStyleSheet("color: #fff; background: #000");
    this->setWindowOpacity(0.8);
    this->updateFont();

    this->setAttribute(Qt::WA_ShowWithoutActivating);
    this->setWindowFlags(Qt::FramelessWindowHint | Qt::WindowStaysOnTopHint |
                         Qt::X11BypassWindowManagerHint | Qt::BypassWindowManagerHint |
                         Qt::SubWindow);

    displayText->setAlignment(Qt::AlignHCenter);
    displayText->setText("tooltip text");
    auto layout = new QVBoxLayout();
    layout->setContentsMargins(10, 5, 10, 5);
    layout->addWidget(displayText);
    this->setLayout(layout);

    this->fontChangedConnection =
        singletons::FontManager::getInstance().fontChanged.connect([this] { this->updateFont(); });
}

TooltipWidget::~TooltipWidget()
{
    this->fontChangedConnection.disconnect();
}

void TooltipWidget::dpiMultiplierChanged(float, float)
{
    this->updateFont();
}

void TooltipWidget::updateFont()
{
    this->setFont(singletons::FontManager::getInstance().getFont(
        singletons::FontManager::Type::MediumSmall, this->getDpiMultiplier()));
}

void TooltipWidget::setText(QString text)
{
    this->displayText->setText(text);
}

void TooltipWidget::moveTo(QWidget *parent, QPoint point)
{
    point.rx() += 16;
    point.ry() += 16;

    this->move(point);
    this->moveIntoDesktopRect(parent);
}

void TooltipWidget::resizeEvent(QResizeEvent *)
{
    this->moveIntoDesktopRect(this);
}

void TooltipWidget::moveIntoDesktopRect(QWidget *parent)
{
    QDesktopWidget *desktop = QApplication::desktop();

    QRect s = desktop->screenGeometry(parent);
    QPoint p = this->pos();

    if (p.x() < s.left()) {
        p.setX(s.left());
    }
    if (p.y() < s.top()) {
        p.setY(s.top());
    }
    if (p.x() + this->width() > s.right()) {
        p.setX(s.right() - this->width());
    }
    if (p.y() + this->height() > s.bottom()) {
        p.setY(s.bottom() - this->height());
    }

    if (p != this->pos()) {
        this->move(p);
    }
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
