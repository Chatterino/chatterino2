#include "widgets/notebook.hpp"
#include "debug/log.hpp"
#include "singletons/thememanager.hpp"
#include "singletons/windowmanager.hpp"
#include "widgets/helper/notebookbutton.hpp"
#include "widgets/helper/notebooktab.hpp"
#include "widgets/settingsdialog.hpp"
#include "widgets/splitcontainer.hpp"
#include "widgets/window.hpp"

#include <QDebug>
#include <QFile>
#include <QFormLayout>
#include <QLayout>
#include <QList>
#include <QShortcut>
#include <QStandardPaths>
#include <QUuid>
#include <QWidget>
#include <boost/foreach.hpp>

namespace chatterino {
namespace widgets {

Notebook::Notebook(Window *parent, bool _showButtons, const std::string &settingPrefix)
    : BaseWidget(parent)
    , parentWindow(parent)
    , settingRoot(fS("{}/notebook", settingPrefix))
    , addButton(this)
    , settingsButton(this)
    , userButton(this)
    , showButtons(_showButtons)
    , tabs(fS("{}/tabs", this->settingRoot))
    , closeConfirmDialog(this)
{
    this->connect(&this->settingsButton, SIGNAL(clicked()), this, SLOT(settingsButtonClicked()));
    this->connect(&this->userButton, SIGNAL(clicked()), this, SLOT(usersButtonClicked()));
    this->connect(&this->addButton, SIGNAL(clicked()), this, SLOT(addPageButtonClicked()));

    this->settingsButton.icon = NotebookButton::IconSettings;

    this->userButton.move(24, 0);
    this->userButton.icon = NotebookButton::IconUser;

    auto &settingsManager = singletons::SettingManager::getInstance();

    settingsManager.hidePreferencesButton.connectSimple([this](auto) { this->performLayout(); });
    settingsManager.hideUserButton.connectSimple([this](auto) { this->performLayout(); });

    this->loadTabs();

    closeConfirmDialog.setText("Are you sure you want to close this tab?");
    closeConfirmDialog.setIcon(QMessageBox::Icon::Question);
    closeConfirmDialog.setStandardButtons(QMessageBox::Yes | QMessageBox::Cancel);
    closeConfirmDialog.setDefaultButton(QMessageBox::Yes);
}

SplitContainer *Notebook::addNewPage()
{
    return this->addPage(CreateUUID().toStdString(), true);
}

SplitContainer *Notebook::addPage(const std::string &uuid, bool select)
{
    auto tab = new NotebookTab(this, uuid);
    auto page = new SplitContainer(this, tab, uuid);

    tab->show();

    if (select || this->pages.count() == 0) {
        this->select(page);
    }

    this->pages.append(page);

    this->performLayout();

    return page;
}

void Notebook::removePage(SplitContainer *page)
{
    if (page->splitCount() > 0 && closeConfirmDialog.exec() != QMessageBox::Yes) {
        return;
    }

    int index = this->pages.indexOf(page);

    if (this->pages.size() == 1) {
        select(nullptr);
    } else if (index == this->pages.count() - 1) {
        select(this->pages[index - 1]);
    } else {
        select(this->pages[index + 1]);
    }

    page->getTab()->deleteLater();
    page->deleteLater();

    this->pages.removeOne(page);

    if (this->pages.size() == 0) {
        this->addNewPage();
    }

    this->performLayout();
}

void Notebook::removeCurrentPage()
{
    if (this->selectedPage == nullptr) {
        return;
    }

    this->removePage(this->selectedPage);
}

void Notebook::select(SplitContainer *page)
{
    if (page == this->selectedPage) {
        return;
    }

    if (page != nullptr) {
        page->setHidden(false);
        page->getTab()->setSelected(true);
        page->getTab()->raise();
    }

    if (this->selectedPage != nullptr) {
        this->selectedPage->setHidden(true);
        this->selectedPage->getTab()->setSelected(false);
        for (auto split : this->selectedPage->getSplits()) {
            split->updateLastReadMessage();
        }
    }

    this->selectedPage = page;

    this->performLayout();
}

void Notebook::selectIndex(int index)
{
    if (index < 0 || index >= this->pages.size()) {
        return;
    }

    this->select(this->pages.at(index));
}

int Notebook::tabCount()
{
    return this->pages.size();
}

SplitContainer *Notebook::tabAt(QPoint point, int &index, int maxWidth)
{
    int i = 0;

    for (auto *page : this->pages) {
        QRect rect = page->getTab()->getDesiredRect();
        rect.setWidth(std::min(maxWidth, rect.width()));

        if (rect.contains(point)) {
            index = i;
            return page;
        }

        i++;
    }

    index = -1;
    return nullptr;
}

void Notebook::rearrangePage(SplitContainer *page, int index)
{
    this->pages.move(this->pages.indexOf(page), index);

    this->performLayout();
}

void Notebook::nextTab()
{
    if (this->pages.size() <= 1) {
        return;
    }

    int index = (this->pages.indexOf(this->selectedPage) + 1) % this->pages.size();

    this->select(this->pages[index]);
}

void Notebook::previousTab()
{
    if (this->pages.size() <= 1) {
        return;
    }

    int index = (this->pages.indexOf(this->selectedPage) - 1);

    if (index < 0) {
        index += this->pages.size();
    }

    this->select(this->pages[index]);
}

void Notebook::performLayout(bool animated)
{
    singletons::SettingManager &settings = singletons::SettingManager::getInstance();

    int x = 0, y = 0;
    float scale = this->getDpiMultiplier();
    bool customFrame = this->parentWindow->hasCustomWindowFrame();

    if (!this->showButtons || settings.hidePreferencesButton || customFrame) {
        this->settingsButton.hide();
    } else {
        this->settingsButton.show();
        x += settingsButton.width();
    }
    if (!this->showButtons || settings.hideUserButton || customFrame) {
        this->userButton.hide();
    } else {
        this->userButton.move(x, 0);
        this->userButton.show();
        x += userButton.width();
    }

    if (customFrame || !this->showButtons ||
        (settings.hideUserButton && settings.hidePreferencesButton)) {
        x += (int)(scale * 2);
    }

    int tabHeight = static_cast<int>(24 * scale);
    bool first = true;

    for (auto &i : this->pages) {
        if (!first &&
            (i == this->pages.last() ? tabHeight : 0) + x + i->getTab()->width() > width()) {
            y += i->getTab()->height();
            i->getTab()->moveAnimated(QPoint(0, y), animated);
            x = i->getTab()->width();
        } else {
            i->getTab()->moveAnimated(QPoint(x, y), animated);
            x += i->getTab()->width();
        }

        first = false;
    }

    this->addButton.move(x, y);

    if (this->selectedPage != nullptr) {
        this->selectedPage->move(0, y + tabHeight);
        this->selectedPage->resize(width(), height() - y - tabHeight);
        this->selectedPage->raise();
    }
}

void Notebook::resizeEvent(QResizeEvent *)
{
    float scale = this->getDpiMultiplier();

    this->settingsButton.resize(static_cast<int>(24 * scale), static_cast<int>(24 * scale));
    this->userButton.resize(static_cast<int>(24 * scale), static_cast<int>(24 * scale));
    this->addButton.resize(static_cast<int>(24 * scale), static_cast<int>(24 * scale));

    for (auto &i : this->pages) {
        i->getTab()->calcSize();
    }

    this->performLayout(false);
}

void Notebook::settingsButtonClicked()
{
    singletons::WindowManager::getInstance().showSettingsDialog();
}

void Notebook::usersButtonClicked()
{
    singletons::WindowManager::getInstance().showAccountSelectPopup(
        this->mapToGlobal(this->userButton.rect().bottomRight()));
}

void Notebook::addPageButtonClicked()
{
    QTimer::singleShot(80, [this] { this->addNewPage(); });
}

void Notebook::loadTabs()
{
    const std::vector<std::string> tabArray = this->tabs.getValue();

    if (tabArray.size() == 0) {
        this->addNewPage();
        return;
    }

    for (const std::string &tabUUID : tabArray) {
        this->addPage(tabUUID);
    }
}

void Notebook::save()
{
    std::vector<std::string> tabArray;

    for (const auto &page : this->pages) {
        tabArray.push_back(page->getUUID());
        page->save();
    }

    this->tabs = tabArray;
}

}  // namespace widgets
}  // namespace chatterino
