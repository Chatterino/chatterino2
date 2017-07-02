#include "widgets/basewidget.hpp"
#include "colorscheme.hpp"

#include <QDebug>
#include <boost/signals2.hpp>

namespace chatterino {
namespace widgets {

BaseWidget::BaseWidget(ColorScheme &_colorScheme, QWidget *parent)
    : QWidget(parent)
    , colorScheme(_colorScheme)
{
    this->init();
}

BaseWidget::BaseWidget(BaseWidget *parent)
    : QWidget(parent)
    , colorScheme(parent->colorScheme)
{
    this->init();
}

void BaseWidget::init()
{
    this->colorScheme.updated.connect([this]() {
        this->refreshTheme();

        this->update();
    });
}

void BaseWidget::refreshTheme()
{
    // Do any color scheme updates here
}

}  // namespace widgets
}  // namespace chatterino
