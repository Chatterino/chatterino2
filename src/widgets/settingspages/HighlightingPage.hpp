#pragma once

#include "widgets/settingspages/SettingsPage.hpp"

#include <QAbstractTableModel>
#include <QTimer>

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
    QTimer disabledUsersChangedTimer;
};

}  // namespace settingspages
}  // namespace widgets
}  // namespace chatterino
