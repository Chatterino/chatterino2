#pragma once

#include "widgets/settingspages/SettingsPage.hpp"

#include <QStringListModel>

namespace chatterino {

class IgnoreUsersPage : public SettingsPage
{
public:
    IgnoreUsersPage();

    void onShow() final;

private:
    QStringListModel userListModel;
};

}  // namespace chatterino
