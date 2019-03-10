#pragma once

#include "widgets/settingspages/SettingsPage.hpp"

#include <QAbstractTableModel>
#include <QTimer>

class QPushButton;
class QListWidget;

namespace chatterino
{
    class HighlightingPage : public SettingsPage
    {
    public:
        HighlightingPage();

    private:
        QTimer disabledUsersChangedTimer_;
    };

}  // namespace chatterino
