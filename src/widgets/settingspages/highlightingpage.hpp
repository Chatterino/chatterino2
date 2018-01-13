#pragma once

#include <QTimer>

#include "widgets/settingspages/settingspage.hpp"

class QPushButton;
class QListWidget;

namespace chatterino {
namespace widgets {
namespace settingspages {

class HighlightingPage : public SettingsPage
{
public:
    HighlightingPage();

private:
    QListWidget *highlightList;
    QPushButton *highlightAdd;
    QPushButton *highlightEdit;
    QPushButton *highlightRemove;

    QTimer disabledUsersChangedTimer;

    void addHighlightTabSignals();
};

}  // namespace settingspages
}  // namespace widgets
}  // namespace chatterino
