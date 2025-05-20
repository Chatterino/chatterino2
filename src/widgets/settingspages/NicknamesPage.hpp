#pragma once

#include "widgets/settingspages/SettingsPage.hpp"

namespace chatterino {

class EditableModelView;

class NicknamesPage : public SettingsPage
{
public:
    NicknamesPage();

private:
    void importNicknames();
    void exportNicknames();
    bool filterElements(const QString &query) override;

private:
    EditableModelView *view_;
};

}  // namespace chatterino
