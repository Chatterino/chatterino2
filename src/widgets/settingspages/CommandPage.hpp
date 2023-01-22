#pragma once

#include "widgets/helper/EditableModelView.hpp"
#include "widgets/settingspages/SettingsPage.hpp"

#include <QTextEdit>
#include <QTimer>

namespace chatterino {

class CommandPage : public SettingsPage
{
public:
    CommandPage();
    bool handleCommandDuplicates(EditableModelView *view);

private:
    QTimer commandsEditTimer_;
    QLabel *duplicateCommandWarning;
};

}  // namespace chatterino
