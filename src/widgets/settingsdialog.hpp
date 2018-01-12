#pragma once

#include "singletons/settingsmanager.hpp"
#include "widgets/accountswitchwidget.hpp"
#include "widgets/helper/settingsdialogtab.hpp"
#include "widgets/settingspages/appearancepage.hpp"

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
#include <QTimer>
#include <QVBoxLayout>
#include <QWidget>
#include <pajlada/settings/setting.hpp>

#include "basewidget.hpp"

namespace chatterino {

namespace widgets {

class SettingsDialog : public BaseWidget
{
public:
    SettingsDialog();

    void select(SettingsDialogTab *tab);

    friend class SettingsDialogTab;

public:
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

    SettingsDialogTab *selectedTab = nullptr;

    void okButtonClicked();
    void cancelButtonClicked();

    //    static void setChildrensFont(QLayout *object, QFont &font, int indent = 0);
};

}  // namespace widgets
}  // namespace chatterino
