#pragma once

#include "colorscheme.hpp"

#include <QWidget>

namespace chatterino {

class ColorScheme;

namespace widgets {

class BaseWidget : public QWidget
{
    Q_OBJECT

public:
    explicit BaseWidget(ColorScheme &_colorScheme, QWidget *parent)
        : QWidget(parent)
        , colorScheme(_colorScheme)
    {
    }

    explicit BaseWidget(BaseWidget *parent)
        : QWidget(parent)
        , colorScheme(parent->colorScheme)
    {
    }

    ColorScheme &colorScheme;
};

}  // namespace widgets
}  // namespace chatterino
