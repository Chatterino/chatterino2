#pragma once

#include "util/LayoutCreator.hpp"
#include "widgets/BaseWindow.hpp"
#include "widgets/helper/ColorButton.hpp"
#include "widgets/helper/QColorPicker.hpp"

#include <pajlada/signals/signal.hpp>

namespace chatterino {

/**
 * @brief A custom color picker dialog.
 *
 * This class exists because QColorPickerDialog did not suit our use case.
 * This dialog provides buttons for recently used and default colors, as well
 * as a color picker widget identical to the one used in QColorPickerDialog.
 */
class ColorPickerDialog : public BaseWindow
{
public:
    /**
     * @brief Create a new color picker dialog that selects the initial color.
     *
     * You can connect to the ::closed signal of this instance to get notified
     * when the dialog is closed.
     */
    ColorPickerDialog(const QColor &initial, QWidget *parent = nullptr);

    ~ColorPickerDialog();

    /**
     * @brief Return the final color selected by the user.
     *
     * Note that this method will always return the invalid color if the dialog
     * is still open, or if the dialog has not been confirmed.
     *
     * @return The color selected by the user, if the dialog was confirmed.
     *         The invalid color, if the dialog has not been confirmed.
     */
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

    /**
     * @brief Update the currently selected color.
     *
     * @param color             Color to update to.
     * @param fromColorPicker   Whether the color update has been triggered by
     *                          one of the color picker widgets. This is needed
     *                          to prevent weird widget behavior.
     */
    void selectColor(const QColor &color, bool fromColorPicker);

    /// Called when the dialog is confirmed.
    void ok();

    // Helper methods for initializing UI elements
    void initRecentColors(LayoutCreator<QWidget> &creator);
    void initDefaultColors(LayoutCreator<QWidget> &creator);
    void initColorPicker(LayoutCreator<QWidget> &creator);
    void initSpinBoxes(LayoutCreator<QWidget> &creator);
    void initHtmlColor(LayoutCreator<QWidget> &creator);
};
}  // namespace chatterino
