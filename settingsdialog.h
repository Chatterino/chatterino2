#ifndef SETTINGSDIALOG_H
#define SETTINGSDIALOG_H

#include "QWidget"
#include "QMainWindow"
#include "QHBoxLayout"
#include "QListView"

class SettingsDialog : QMainWindow
{
public:
    SettingsDialog();

private:
    QListView tabs;
    QHBoxLayout hbox;

    void addTab(QWidget* widget, QString title, QString imageRes);
};

#endif // SETTINGSDIALOG_H
