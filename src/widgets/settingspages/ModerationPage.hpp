#pragma once

#include <QTimer>

#include "widgets/settingspages/SettingsPage.hpp"

class QTabWidget;
class QPushButton;

namespace chatterino {

class ModerationPage : public SettingsPage
{
public:
    ModerationPage();

    void selectModerationActions();

private:
    QTimer itemsChangedTimer_;
    QTabWidget *tabWidget_{};
};

}  // namespace chatterino
