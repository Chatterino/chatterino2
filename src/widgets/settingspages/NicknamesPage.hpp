#pragma once

#include "widgets/settingspages/SettingsPage.hpp"

namespace chatterino {

class NicknamesPage : public SettingsPage
{
public:
    NicknamesPage();

private:
    void importNicknames();
    void exportNicknames();
};

}  // namespace chatterino
