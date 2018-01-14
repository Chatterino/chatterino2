#pragma once

#include "basewindow.hpp"

#include <QPushButton>
#include <QStackedLayout>
#include <QVBoxLayout>
#include <QWidget>
#include <pajlada/settings/setting.hpp>

namespace chatterino {
namespace widgets {

namespace settingspages {
class SettingsPage;
}

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
    virtual void dpiMultiplierChanged(float oldDpi, float newDpi) override;

private:
    void refresh();
    static SettingsDialog *handle;

    struct {
        QWidget *tabContainerContainer;
        QVBoxLayout *tabContainer;
        QStackedLayout *pageStack;
        QPushButton *okButton;
        QPushButton *cancelButton;
    } ui;

    std::vector<SettingsDialogTab *> tabs;

    void initUi();
    void addTabs();
    void addTab(settingspages::SettingsPage *page, Qt::Alignment alignment = Qt::AlignTop);

    void select(SettingsDialogTab *tab);

    SettingsDialogTab *selectedTab = nullptr;

    void okButtonClicked();
    void cancelButtonClicked();

    friend class SettingsDialogTab;

    //    static void setChildrensFont(QLayout *object, QFont &font, int indent = 0);
};

}  // namespace widgets
}  // namespace chatterino
