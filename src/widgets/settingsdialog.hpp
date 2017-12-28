#pragma once

#include "settingsmanager.hpp"
#include "widgets/accountswitchwidget.hpp"
#include "widgets/helper/settingsdialogtab.hpp"

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

#include "basewidget.hpp"

namespace chatterino {

namespace widgets {

class SettingsDialog : public BaseWidget
{
    SettingsDialog();

    void select(SettingsDialogTab *tab);

    friend class SettingsDialogTab;

public:
    enum class PreferredTab {
        NoPreference,
        Accounts,
    };

    static void showDialog(PreferredTab preferredTab = PreferredTab::NoPreference);

protected:
    virtual void dpiMultiplierChanged(float oldDpi, float newDpi) override;

private:
    void refresh();

    std::vector<SettingsDialogTab *> tabs;

    pajlada::Settings::Setting<int> usernameDisplayMode;

    struct {
        QVBoxLayout tabs;
        QVBoxLayout vbox;
        QHBoxLayout hbox;
        QWidget tabWidget;
        QStackedLayout pageStack;
        QDialogButtonBox buttonBox;
        QPushButton okButton;
        QPushButton cancelButton;

        AccountSwitchWidget *accountSwitchWidget = nullptr;
    } ui;

    void addTab(QBoxLayout *layout, QString title, QString imageRes);
    void addTabs();

    QVBoxLayout *createAccountsTab();
    QVBoxLayout *createAppearanceTab();
    QVBoxLayout *createMessagesTab();
    QVBoxLayout *createBehaviourTab();
    QVBoxLayout *createCommandsTab();
    QVBoxLayout *createEmotesTab();
    QVBoxLayout *createIgnoredUsersTab();
    QVBoxLayout *createIgnoredMessagesTab();
    QVBoxLayout *createLinksTab();
    QVBoxLayout *createLogsTab();
    QVBoxLayout *createHighlightingTab();
    QVBoxLayout *createWhispersTab();

    SettingsDialogTab *selectedTab = nullptr;

    QListWidget *globalHighlights;

    /// Widget creation helpers
    QVBoxLayout *createTabLayout();
    QCheckBox *createCheckbox(const QString &title, Setting<bool> &setting);
    QCheckBox *createCheckbox(const QString &title, pajlada::Settings::Setting<bool> &setting);
    QHBoxLayout *createCombobox(const QString &title, pajlada::Settings::Setting<int> &setting,
                                QStringList items,
                                std::function<void(QString, pajlada::Settings::Setting<int> &)> cb);
    QHBoxLayout *createCombobox(
        const QString &title, pajlada::Settings::Setting<std::string> &setting, QStringList items,
        std::function<void(QString, pajlada::Settings::Setting<std::string> &)> cb);
    QLineEdit *createLineEdit(pajlada::Settings::Setting<std::string> &setting);

    void okButtonClicked();
    void cancelButtonClicked();

    std::vector<pajlada::Signals::ScopedConnection> managedConnections;

    static void setChildrensFont(QLayout *object, QFont &font, int indent = 0);
};

}  // namespace widgets
}  // namespace chatterino
