#pragma once

#include "settingsmanager.hpp"
#include "settingssnapshot.hpp"
#include "widgets/settingsdialogtab.hpp"

#include <QButtonGroup>
#include <QCheckBox>
#include <QComboBox>
#include <QDialogButtonBox>
#include <QHBoxLayout>
#include <QListView>
#include <QListWidget>
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

    pajlada::Settings::Setting<int> usernameDisplayMode;

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

    QListWidget *globalHighlights;

    /// Widget creation helpers
    QCheckBox *createCheckbox(const QString &title, Setting<bool> &setting);
    QCheckBox *createCheckbox(const QString &title, pajlada::Settings::Setting<bool> &setting);
    QHBoxLayout *createCombobox(const QString &title, pajlada::Settings::Setting<int> &setting,
                                QStringList items,
                                std::function<void(QString, pajlada::Settings::Setting<int> &)> cb);
    QHBoxLayout *createCombobox(const QString &title, pajlada::Settings::Setting<std::string> &setting,
                                QStringList items,
                                std::function<void(QString, pajlada::Settings::Setting<std::string> &)> cb);
    QLineEdit *createLineEdit(pajlada::Settings::Setting<std::string> &setting);

    void okButtonClicked();
    void cancelButtonClicked();
};

}  // namespace widgets
}  // namespace chatterino
