#pragma once

#include "widgets/settingspages/SettingsPage.hpp"

#include <QTimer>

namespace chatterino {

class EditableModelView;

class CommandPage : public SettingsPage
{
public:
    CommandPage();
    bool filterElements(const QString &query) override;

private:
    QTimer commandsEditTimer_;
    EditableModelView *view;
};

}  // namespace chatterino
