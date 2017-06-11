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

namespace chatterino {
namespace widgets {

class SettingsDialog : public QWidget
{
public:
    SettingsDialog();

    void select(SettingsDialogTab *tab);

    static void showDialog();

private:
    SettingsSnapshot _snapshot;

    QVBoxLayout _tabs;
    QVBoxLayout _vbox;
    QHBoxLayout _hbox;
    QStackedLayout _pageStack;
    QDialogButtonBox _buttonBox;
    QPushButton _okButton;
    QPushButton _cancelButton;

    void addTab(QLayout *layout, QString title, QString imageRes);

    void addTabs();

    SettingsDialogTab *_selectedTab = nullptr;

    /// Widget creation helpers
    QCheckBox *createCheckbox(const QString &title, Setting<bool> &setting);

    void okButtonClicked();
    void cancelButtonClicked();
};

}  // namespace widgets
}  // namespace chatterino
