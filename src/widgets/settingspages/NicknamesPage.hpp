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
    void importNicknames();
    void exportNicknames();
};

}  // namespace chatterino
