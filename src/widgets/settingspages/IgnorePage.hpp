#pragma once

#include "widgets/settingspages/SettingsPage.hpp"

#include <QStringListModel>

namespace chatterino {

class IgnoresPage : public SettingsPage
{
public:
    IgnoresPage();

    void onShow() final;

private:
    QStringListModel userListModel;
};

}  // namespace chatterino
