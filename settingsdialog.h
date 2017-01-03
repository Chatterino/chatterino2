#ifndef SETTINGSDIALOG_H
#define SETTINGSDIALOG_H

#include "QWidget"
#include "QMainWindow"
#include "QHBoxLayout"
#include "QVBoxLayout"
#include "QListView"
#include "QButtonGroup"
#include "QPushButton"
#include "QDialogButtonBox"
#include "QStackedLayout"
#include "settingsdialogtab.h"
#include "QCheckBox"

class SettingsDialog : public QWidget
{
public:
    SettingsDialog();

    void select(SettingsDialogTab* tab);

private:
    QVBoxLayout tabs;
    QVBoxLayout vbox;
    QHBoxLayout hbox;
    QStackedLayout pageStack;
    QDialogButtonBox buttonBox;
    QPushButton okButton;
    QPushButton cancelButton;

    void addTab(QLayout* layout, QString title, QString imageRes);

    void addTabs();

    SettingsDialogTab* selectedTab = NULL;

    QCheckBox* createCheckbox(QString title, QString settingsId);
};

#endif // SETTINGSDIALOG_H
