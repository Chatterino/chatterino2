#include "widgets/helper/LiveIndicator.hpp"

#include "singletons/Theme.hpp"
#include "util/Helpers.hpp"

#include <QPainter>
#include <QString>

namespace chatterino {

using namespace Qt::Literals;

LiveIndicator::LiveIndicator(int paddingRight, QWidget *parent)
    : BaseWidget(parent)
    , paddingRight(paddingRight)
{
    this->setMinimumHeight(5);  // fixed min height for the circle to fit
    this->updateScale();
}

void LiveIndicator::setViewers(std::optional<int> viewers)
{
    this->viewers = viewers;
    if (this->viewers)
    {
        this->setToolTip(
            u"Live with %1 viewers"_s.arg(localizeNumbers(*viewers)));
    }
    else
    {
        this->setToolTip({});
    }
    this->updateScale();
}

void LiveIndicator::scaleChangedEvent(float /*newScale*/)
{
    this->updateScale();
}

void LiveIndicator::paintEvent(QPaintEvent * /*event*/)
{
    if (!this->viewers)
    {
        return;
    }
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
        QPoint{1, 0},
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
    qreal width = this->paddingRight;
    if (this->viewers)
    {
        width += 6;  // 1dp left padding + 5dp for the circle
    }
    // for hover and tooltip
    this->setMouseTracking(this->viewers.has_value());
    this->setFixedWidth(static_cast<int>(width * this->scale()));

    this->update();
}

}  // namespace chatterino
