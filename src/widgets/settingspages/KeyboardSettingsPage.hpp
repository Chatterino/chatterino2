#pragma once

#include "widgets/settingspages/SettingsPage.hpp"

namespace chatterino {

class HotkeyModel;
class EditableModelView;

class KeyboardSettingsPage : public SettingsPage
{
public:
    KeyboardSettingsPage();

private:
    void tableCellClicked(const QModelIndex &clicked, EditableModelView *view,
                          HotkeyModel *model);
};

}  // namespace chatterino
