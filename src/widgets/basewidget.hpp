#pragma once

#include <QWidget>

namespace chatterino {

class ColorScheme;

namespace widgets {

class BaseWidget : public QWidget
{
    Q_OBJECT

public:
    explicit BaseWidget(ColorScheme &_colorScheme, QWidget *parent);

    explicit BaseWidget(BaseWidget *parent);

    ColorScheme &colorScheme;

private:
    void init();

    virtual void refreshTheme();
};

}  // namespace widgets
}  // namespace chatterino
