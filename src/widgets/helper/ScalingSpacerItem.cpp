#include "widgets/helper/ScalingSpacerItem.hpp"

#include "singletons/Settings.hpp"

namespace chatterino {

ScalingSpacerItem *ScalingSpacerItem::horizontal(int baseWidth)
{
    return new ScalingSpacerItem({baseWidth, 0}, QSizePolicy::Fixed,
                                 QSizePolicy::Minimum);
}

ScalingSpacerItem *ScalingSpacerItem::vertical(int baseHeight)
{
    return new ScalingSpacerItem({0, baseHeight}, QSizePolicy::Minimum,
                                 QSizePolicy::Fixed);
}

ScalingSpacerItem::ScalingSpacerItem(QSize baseSize, QSizePolicy::Policy horiz,
                                     QSizePolicy::Policy vert)
    : QSpacerItem(
          qRound(static_cast<float>(baseSize.width()) * getSettings()->uiScale),
          qRound(static_cast<float>(baseSize.height()) *
                 getSettings()->uiScale),
          horiz, vert)
    , baseSize(baseSize)
    , scaledSize(baseSize * getSettings()->uiScale)
    , horizontalPolicy(horiz)
    , verticalPolicy(vert)
{
    getSettings()->uiScale.connect(
        [this] {
            this->refresh();
        },
        this->connections, false);
}

void ScalingSpacerItem::refresh()
{
    auto nextSize = this->baseSize * getSettings()->uiScale;
    if (nextSize == this->scaledSize)
    {
        return;
    }
    this->scaledSize = nextSize;

    this->changeSize(this->scaledSize.width(), this->scaledSize.height(),
                     this->horizontalPolicy, this->verticalPolicy);
    this->invalidate();
}

}  // namespace chatterino
