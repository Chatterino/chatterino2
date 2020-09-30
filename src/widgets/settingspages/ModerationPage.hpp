#pragma once

#include <QTimer>

#include "widgets/settingspages/SettingsPage.hpp"

class QTabWidget;
class QPushButton;

namespace chatterino {

template <typename X>
class LayoutCreator;

class ModerationPage : public SettingsPage
{
public:
    ModerationPage();

    void selectModerationActions();

private:
    void addModerationButtonSettings(LayoutCreator<QTabWidget> &);

    QTimer itemsChangedTimer_;
    QTabWidget *tabWidget_{};

    std::vector<QLineEdit *> durationInputs_;
    std::vector<QComboBox *> unitInputs_;
};

}  // namespace chatterino
