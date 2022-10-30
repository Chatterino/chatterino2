#pragma once

#include "widgets/helper/EditableModelView.hpp"
#include "widgets/settingspages/SettingsPage.hpp"

#include <QStringListModel>

class QVBoxLayout;

namespace chatterino {

class NicknamesPage : public SettingsPage
{
public:
    NicknamesPage();
};

}  // namespace chatterino
