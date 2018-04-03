#pragma once

#include "widgets/settingspages/settingspage.hpp"

#include <QTimer>

namespace chatterino {
namespace widgets {
namespace settingspages {

class IgnoreMessagesPage : public SettingsPage
{
public:
    IgnoreMessagesPage();

    QTimer keywordsUpdated;
};

}  // namespace settingspages
}  // namespace widgets
}  // namespace chatterino
