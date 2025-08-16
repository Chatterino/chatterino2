#include "widgets/helper/LiveIndicator.hpp"

#include "singletons/Theme.hpp"
#include "util/Helpers.hpp"

#include <QPainter>
#include <QString>

namespace chatterino {

using namespace Qt::Literals;

LiveIndicator::LiveIndicator(QWidget *parent)
    : BaseWidget(parent)
{
    this->setMinimumHeight(5);     // fixed min height for the circle to fit
    this->setMouseTracking(true);  // for hover and tooltip
    this->updateScale();
}

void LiveIndicator::setViewers(int viewers)
{
    this->setToolTip(u"Live with %1 viewers"_s.arg(localizeNumbers(viewers)));
    this->updateScale();
}

void LiveIndicator::scaleChangedEvent(float /*newScale*/)
{
    this->updateScale();
}

void LiveIndicator::paintEvent(QPaintEvent * /*event*/)
{
    QPainter painter(this);
    QColor color = getTheme()->tabs.liveIndicator;
    // Indicate that there's a tooltip here
    if (this->hovered)
    {
        if (getTheme()->isLightTheme())
        {
            color = color.darker(150);
        }
        else
        {
            color = color.lighter(150);
        }
    }

    painter.setBrush(color);
    painter.setPen(Qt::NoPen);
    painter.setRenderHint(QPainter::Antialiasing);
    painter.drawEllipse(QRect{
        QPoint{0, 0},
        QSize{5, 5} * this->scale(),
    });
}

void LiveIndicator::enterEvent(QEnterEvent * /*event*/)
{
    this->hovered = true;
    this->update();
}
void LiveIndicator::leaveEvent(QEvent * /*event*/)
{
    this->hovered = false;
    this->update();
}

void LiveIndicator::updateScale()
{
    this->setFixedWidth(qRound(6 * this->scale()));

    this->update();
}

}  // namespace chatterino
