#include "SplitContainer.hpp"

#include "ui/FlexLayout.hpp"
#include "ui/Split.hpp"

namespace chatterino::ui
{
    SplitContainer::SplitContainer(Application& app)
    {
        auto layout = new FlexLayout();
        layout->addWidget(new Split(app));
        layout->addWidget(new Split(app));
        layout->addWidget(new Split(app));
        layout->addWidgetRelativeTo(
            new Split(app), layout->itemAt(1)->widget(), Direction::Above);
        this->setLayout(layout);
    }
}  // namespace chatterino::ui
