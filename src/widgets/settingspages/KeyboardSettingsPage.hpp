#pragma once

#include "widgets/helper/EditableModelView.hpp"
#include "widgets/settingspages/SettingsPage.hpp"

namespace chatterino {

class KeyboardSettingsPage : public SettingsPage
{
public:
    KeyboardSettingsPage();

private:
    void tableCellClicked(const QModelIndex &clicked, EditableModelView *view);
};

}  // namespace chatterino
