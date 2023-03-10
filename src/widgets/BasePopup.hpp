#pragma once

#include "common/FlagsEnum.hpp"
#include "widgets/BaseWindow.hpp"

class QDialogButtonBox;

namespace chatterino {

class BasePopup : public BaseWindow
{
public:
    explicit BasePopup(FlagsEnum<BaseWindow::Flags> flags_ = None,
                       QWidget *parent = nullptr);

protected:
    void keyPressEvent(QKeyEvent *e) override;

    // handleEscape is a helper function for clicking the "Reject" role button of a button box when the Escape button is pressed
    bool handleEscape(QKeyEvent *e, QDialogButtonBox *buttonBox);

    // handleEnter is a helper function for clicking the "Accept" role button of a button box when Return or Enter is pressed
    bool handleEnter(QKeyEvent *e, QDialogButtonBox *buttonBox);
};

}  // namespace chatterino
