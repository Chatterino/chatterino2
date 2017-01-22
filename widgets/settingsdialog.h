#ifndef SETTINGSDIALOG_H
#define SETTINGSDIALOG_H

#include "settings/settings.h"
#include "widgets/settingsdialogtab.h"

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

private:
    QVBoxLayout tabs;
    QVBoxLayout vbox;
    QHBoxLayout hbox;
    QStackedLayout pageStack;
    QDialogButtonBox buttonBox;
    QPushButton okButton;
    QPushButton cancelButton;

    void addTab(QLayout *layout, QString title, QString imageRes);

    void addTabs();

    SettingsDialogTab *selectedTab = NULL;

    QCheckBox *createCheckbox(const QString &title,
                              settings::BoolSetting &setting);
};
}
}

#endif  // SETTINGSDIALOG_H
