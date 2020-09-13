#pragma once

#include "common/Channel.hpp"
#include "widgets/BasePopup.hpp"
#include "widgets/dialogs/switcher/QuickSwitcherModel.hpp"
#include "widgets/splits/Split.hpp"
#include "widgets/splits/SplitContainer.hpp"

#include <functional>

namespace chatterino {

class GenericListView;

class QuickSwitcherPopup : public BasePopup
{
public:
    /**
     * @brief   Construct a new QuickSwitcherPopup.
     *
     * @param   parent  Parent widget of the popup. The popup will be placed
     *                  in the center of the parent widget.
     */
    explicit QuickSwitcherPopup(QWidget *parent = nullptr);

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

    void initWidgets();
};

}  // namespace chatterino
