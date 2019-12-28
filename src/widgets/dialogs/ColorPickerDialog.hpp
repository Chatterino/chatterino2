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

    ~ColorPickerDialog();

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
        std::array<QColSpinBox *, 4> spinBoxes;
        QLineEdit *htmlEdit;
    } ui_;

    enum SpinBox : size_t { RED = 0, GREEN = 1, BLUE = 2, ALPHA = 3, END };

    static const size_t MAX_RECENT_COLORS = 10;

    static const size_t RECENT_COLORS_PER_ROW = 5;
    static const size_t DEFAULT_COLORS_PER_ROW = 5;

    QColor color_;
    bool dialogConfirmed_;
    QRegularExpressionValidator *htmlColorValidator_{};

    void selectColor(const QColor &color, bool fromColorPicker);
    void ok();

    // Helper methods for initializing UI elements

    void initRecentColors(LayoutCreator<QWidget> &creator);
    void initDefaultColors(LayoutCreator<QWidget> &creator);
    void initColorPicker(LayoutCreator<QWidget> &creator);
    void initSpinBoxes(LayoutCreator<QWidget> &creator);
    void initHtmlColor(LayoutCreator<QWidget> &creator);
};
}  // namespace chatterino
