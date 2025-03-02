#pragma once

#include "widgets/settingspages/SettingsPage.hpp"

class QComboBox;

namespace chatterino {

class EditableModelView;

class NotificationPage : public SettingsPage
{
public:
    NotificationPage();
    bool filterElements(const QString &query) override;

private:
    QComboBox *createToastReactionComboBox();
    EditableModelView *view_;
};

}  // namespace chatterino
