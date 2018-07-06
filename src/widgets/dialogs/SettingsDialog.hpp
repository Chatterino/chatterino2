#pragma once

#include "widgets/BaseWindow.hpp"

#include <QPushButton>
#include <QStackedLayout>
#include <QVBoxLayout>
#include <QWidget>
#include <pajlada/settings/setting.hpp>

namespace chatterino {

class SettingsPage;
class SettingsDialogTab;

class SettingsDialog : public BaseWindow
{
public:
    SettingsDialog();

    static SettingsDialog *getHandle();  // may be NULL

    enum class PreferredTab {
        NoPreference,
        Accounts,
    };

    static void showDialog(PreferredTab preferredTab = PreferredTab::NoPreference);

protected:
    virtual void scaleChangedEvent(float newDpi) override;
    virtual void themeChangedEvent() override;

private:
    static SettingsDialog *handle;

    void refresh();

    void initUi();
    void addTabs();
    void addTab(SettingsPage *page, Qt::Alignment alignment = Qt::AlignTop);
    void selectTab(SettingsDialogTab *tab);

    void onOkClicked();
    void onCancelClicked();

    struct {
        QWidget *tabContainerContainer;
        QVBoxLayout *tabContainer;
        QStackedLayout *pageStack;
        QPushButton *okButton;
        QPushButton *cancelButton;
    } ui_;
    std::vector<SettingsDialogTab *> tabs_;
    SettingsDialogTab *selectedTab_ = nullptr;

    friend class SettingsDialogTab;
};

}  // namespace chatterino
