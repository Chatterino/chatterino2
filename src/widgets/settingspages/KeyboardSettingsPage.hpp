#pragma once

#include "widgets/helper/EditableModelView.hpp"
#include "widgets/settingspages/SettingsPage.hpp"

namespace chatterino {

class HotkeyModel;

class KeyboardSettingsPage : public SettingsPage
{
public:
    KeyboardSettingsPage();

private:
    void tableCellClicked(const QModelIndex &clicked, EditableModelView *view,
                          HotkeyModel *model);
};

}  // namespace chatterino
