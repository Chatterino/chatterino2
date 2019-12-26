#pragma once

#include "util/LayoutCreator.hpp"
#include "widgets/BaseWindow.hpp"
#include "widgets/helper/ColorButton.hpp"
#include "widgets/helper/QColorPicker.hpp"

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
        std::vector<ColorButton *> defaultColors;
        ColorButton *selectedColor;
        QColorPicker *colorPicker;
        QColorLuminancePicker *luminancePicker;

        struct {
            QColSpinBox *red;
            QColSpinBox *green;
            QColSpinBox *blue;
            QColSpinBox *alpha;
        } spinBoxes;
    } ui_;

    QColor color_;

    bool dialogConfirmed_;

    void selectColor(const QColor &color, bool fromColorPicker);
    void ok();

    void initSpinBoxes(LayoutCreator<QWidget> &creator);
};
}  // namespace chatterino
