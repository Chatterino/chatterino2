#pragma once

#include "settingsmanager.hpp"
#include "settingssnapshot.hpp"
#include "widgets/settingsdialogtab.hpp"

#include <QButtonGroup>
#include <QCheckBox>
#include <QDialogButtonBox>
#include <QHBoxLayout>
#include <QListView>
#include <QMainWindow>
#include <QPushButton>
#include <QStackedLayout>
#include <QVBoxLayout>
#include <QWidget>
#include <pajlada/settings/setting.hpp>

namespace chatterino {

namespace widgets {

class SettingsDialog : public QWidget
{
public:
    SettingsDialog();

    void select(SettingsDialogTab *tab);

    static void showDialog();

private:
    SettingsSnapshot snapshot;

    struct {
        QVBoxLayout tabs;
        QVBoxLayout vbox;
        QHBoxLayout hbox;
        QStackedLayout pageStack;
        QDialogButtonBox buttonBox;
        QPushButton okButton;
        QPushButton cancelButton;
    } ui;

    void addTab(QLayout *layout, QString title, QString imageRes);

    void addTabs();

    SettingsDialogTab *selectedTab = nullptr;

    /// Widget creation helpers
    QCheckBox *createCheckbox(const QString &title, Setting<bool> &setting);
    QCheckBox *createCheckbox(const QString &title, pajlada::Settings::Setting<bool> &setting);

    void okButtonClicked();
    void cancelButtonClicked();
};

}  // namespace widgets
}  // namespace chatterino
