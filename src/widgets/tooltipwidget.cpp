#include "tooltipwidget.hpp"
#include "singletons/thememanager.hpp"
#include "singletons/fontmanager.hpp"

#include <QStyle>
#include <QVBoxLayout>

namespace chatterino {
namespace widgets {

TooltipWidget::TooltipWidget(BaseWidget *parent)
    : BaseWidget(parent)
    , displayText(new QLabel())
{
    QColor black(0, 0, 0);
    QColor white(255, 255, 255);

    QPalette palette;
    palette.setColor(QPalette::WindowText, white);
    palette.setColor(QPalette::Background, black);
    this->setPalette(palette);
    this->displayText->setStyleSheet("color: #ffffff");
    this->setWindowOpacity(0.8);
    this->setFont(singletons::FontManager::getInstance().getFont(singletons::FontManager::Type::MediumSmall,
                                                     this->getDpiMultiplier()));

    this->setAttribute(Qt::WA_ShowWithoutActivating);
    this->setWindowFlags(Qt::FramelessWindowHint | Qt::WindowStaysOnTopHint | Qt::X11BypassWindowManagerHint);

    displayText->setAlignment(Qt::AlignHCenter);
    displayText->setText("lmao xD");
    auto layout = new QVBoxLayout();
    layout->setContentsMargins(10, 5, 10, 5);
    layout->addWidget(displayText);
    this->setLayout(layout);

    singletons::FontManager::getInstance().fontChanged.connect([this] {
        this->setFont(singletons::FontManager::getInstance().getFont(singletons::FontManager::Type::MediumSmall,
                                                         this->getDpiMultiplier()));
    });
}

void TooltipWidget::setText(QString text)
{
    this->displayText->setText(text);
}

void TooltipWidget::moveTo(QPoint point)
{
    point.rx() += 16;
    point.ry() += 16;
    this->move(point);
}

}  // namespace widgets
}  // namespace chatterino
