#pragma once

#include "widgets/settingspages/SettingsPage.hpp"

#include <QTimer>

class QTabWidget;
class QPushButton;

namespace chatterino {

class EditableModelView;

class ModerationPage : public SettingsPage
{
public:
    ModerationPage();

    void selectModerationActions();
    bool filterElements(const QString &query) override;

private:
    void addModerationButtonSettings(QTabWidget *);

    QTimer itemsChangedTimer_;
    QTabWidget *tabWidget_{};

    EditableModelView *viewLogs_;
    EditableModelView *viewModerationButtons_;

    std::vector<QLineEdit *> durationInputs_;
    std::vector<QComboBox *> unitInputs_;
};

}  // namespace chatterino
