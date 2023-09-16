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
    virtual void themeChangedEvent() override;

public slots:
    void updateSuggestions(const QString &text);

private:
    static const QSize MINIMUM_SIZE;

    struct {
        QLineEdit *searchEdit{};
        GenericListView *list{};
    } ui_;

    QuickSwitcherModel switcherModel_;

    Window *window{};

    void initWidgets();
};

}  // namespace chatterino
