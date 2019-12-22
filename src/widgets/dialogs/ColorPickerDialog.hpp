#pragma once

#include "widgets/BaseWindow.hpp"
#include "widgets/helper/ColorButton.hpp"

#include <pajlada/signals/signal.hpp>

namespace chatterino {

class ColorPickerDialog : public BaseWindow
{
public:
    ColorPickerDialog(const QColor &initial, QWidget *parent = nullptr);

    QColor selectedColor() const;

    pajlada::Signals::NoArgSignal closed;

protected:
    void closeEvent(QCloseEvent *);

private:
    struct {
        std::vector<ColorButton *> recentColors;
        ColorButton *selectedColor;
    } ui_;

    bool hasSelectedColor_;

    void ok();
};
}  // namespace chatterino
