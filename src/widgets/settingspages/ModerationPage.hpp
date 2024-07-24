#pragma once

#include "widgets/settingspages/SettingsPage.hpp"

#include <QTimer>

class QTabWidget;
class QPushButton;

namespace chatterino {

class ModerationPage : public SettingsPage
{
public:
    ModerationPage();

    void selectModerationActions();

private:
    void addModerationButtonSettings(QTabWidget *);

    QTimer itemsChangedTimer_;
    QTabWidget *tabWidget_{};

    std::vector<QLineEdit *> durationInputs_;
    std::vector<QComboBox *> unitInputs_;
};

}  // namespace chatterino
