#pragma once

#include "widgets/settingspages/settingspage.hpp"

#include <QStringListModel>

namespace chatterino {
namespace widgets {
namespace settingspages {

class IgnoreUsersPage : public SettingsPage
{
public:
    IgnoreUsersPage();

    void onShow() final;

private:
    QStringListModel userListModel;
};

}  // namespace settingspages
}  // namespace widgets
}  // namespace chatterino
