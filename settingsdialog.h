#ifndef SETTINGSDIALOG_H
#define SETTINGSDIALOG_H

#include "settingsdialogtab.h"

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

    QCheckBox *createCheckbox(QString title, QString settingsId);
};

#endif  // SETTINGSDIALOG_H
