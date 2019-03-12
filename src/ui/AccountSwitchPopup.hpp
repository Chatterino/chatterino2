#pragma once

#include "widgets/AccountSwitchWidget.hpp"

#include "ab/BaseWindow.hpp"

namespace chatterino::ui
{
    class AccountSwitchPopup : public ab::Popup
    {
        Q_OBJECT

    public:
        AccountSwitchPopup();
    };
}  // namespace chatterino::ui
