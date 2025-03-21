#pragma once

#include "widgets/BasePopup.hpp"
#include "widgets/dialogs/switcher/QuickSwitcherModel.hpp"

#include <QLineEdit>

#include <functional>

namespace chatterino {

class GenericListView;
class Window;

class QuickSwitcherPopup : public BasePopup
{
public:
    /**
     * @brief   Construct a new QuickSwitcherPopup.
     *
     * @param   parent  Parent window of the popup. The popup will be placed
     *                  in the center of the window.
     */
    explicit QuickSwitcherPopup(Window *parent);

protected:
    void themeChangedEvent() override;

public Q_SLOTS:
    void updateSuggestions(const QString &text);

private:
    constexpr static const QSize MINIMUM_SIZE{500, 300};

    struct {
        QLineEdit *searchEdit{};
        GenericListView *list{};
    } ui_;

    QuickSwitcherModel switcherModel_;

    Window *window{};

    void initWidgets();
};

}  // namespace chatterino
