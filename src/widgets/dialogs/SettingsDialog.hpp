#pragma once

#include "widgets/BaseWindow.hpp"

#include <QPushButton>
#include <QStackedLayout>
#include <QVBoxLayout>
#include <QWidget>
#include <functional>
#include <pajlada/settings/setting.hpp>
#include "widgets/helper/SettingsDialogTab.hpp"

class QLineEdit;

namespace chatterino {

class SettingsPage;
class SettingsDialogTab;
class ModerationPage;

class PageHeader : public QFrame
{
    Q_OBJECT
};

enum class SettingsDialogPreference {
    NoPreference,
    Accounts,
    ModerationActions,
};

class SettingsDialog : public BaseWindow
{
    SettingsDialog();

public:
    static void showDialog(SettingsDialogPreference preferredTab =
                               SettingsDialogPreference::NoPreference);

protected:
    virtual void scaleChangedEvent(float newDpi) override;
    virtual void themeChangedEvent() override;
    virtual void showEvent(QShowEvent *) override;

private:
    void refresh();

    void initUi();
    SettingsDialogTab *tab(SettingsTabId id);
    void addTabs();
    void addTab(std::function<SettingsPage *()> page, const QString &name,
                const QString &iconPath, SettingsTabId id = {},
                Qt::Alignment alignment = Qt::AlignTop);
    void selectTab(SettingsDialogTab *tab, const bool byUser = true);
    void selectTab(SettingsTabId id);
    void filterElements(const QString &query);

    void onOkClicked();
    void onCancelClicked();

    struct {
        QWidget *tabContainerContainer{};
        QVBoxLayout *tabContainer{};
        QStackedLayout *pageStack{};
        QPushButton *okButton{};
        QPushButton *cancelButton{};
        QLineEdit *search{};
    } ui_;
    std::vector<SettingsDialogTab *> tabs_;
    SettingsDialogTab *selectedTab_{};
    SettingsDialogTab *lastSelectedByUser_{};

    friend class SettingsDialogTab;
};

}  // namespace chatterino
