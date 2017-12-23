#include "tooltipwidget.hpp"
#include "colorscheme.hpp"
#include "fontmanager.hpp"

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
    this->setWindowOpacity(0.8);
    this->setFont(FontManager::getInstance().getFont(FontManager::Type::MediumSmall,
                                                     this->getDpiMultiplier()));

    this->setAttribute(Qt::WA_ShowWithoutActivating);
    this->setWindowFlags(Qt::FramelessWindowHint | Qt::WindowStaysOnTopHint);

    displayText->setAlignment(Qt::AlignHCenter);
    auto layout = new QVBoxLayout();
    layout->setContentsMargins(10, 5, 10, 5);
    layout->addWidget(displayText);
    this->setLayout(layout);

    FontManager::getInstance().fontChanged.connect([this] {
        this->setFont(FontManager::getInstance().getFont(FontManager::Type::MediumSmall,
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
