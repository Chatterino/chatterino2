#pragma once

#include "widgets/settingspages/SettingsPage.hpp"

#include <QTimer>

class QTabWidget;
class QPushButton;

namespace chatterino {

template <typename X>
class LayoutCreator;
class EditableModelView;

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

    void openImageDialog(const QModelIndex &clicked, EditableModelView *view);
};

}  // namespace chatterino
