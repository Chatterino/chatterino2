#pragma once

#include <QTimer>
#include "widgets/settingspages/settingspage.hpp"

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
