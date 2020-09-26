#pragma once

#include "widgets/settingspages/SettingsPage.hpp"

namespace chatterino {

class AdvancedPage : public SettingsPage
{
public:
    AdvancedPage();

private:
    // list needed for dynamic timeout settings
    std::vector<QLineEdit *> durationInputs_;
    std::vector<QComboBox *> unitInputs_;

};

}  // namespace chatterino
