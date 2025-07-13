#pragma once

#include "widgets/settingspages/SettingsPage.hpp"

namespace chatterino {

class EditableModelView;

class NicknamesPage : public SettingsPage
{
public:
    NicknamesPage();
    bool filterElements(const QString &query) override;

private:
    EditableModelView *view_;
};

}  // namespace chatterino
