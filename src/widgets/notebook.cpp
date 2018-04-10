#include "widgets/notebook.hpp"
#include "debug/log.hpp"
#include "singletons/thememanager.hpp"
#include "singletons/windowmanager.hpp"
#include "widgets/helper/notebookbutton.hpp"
#include "widgets/helper/notebooktab.hpp"
#include "widgets/helper/shortcut.hpp"
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

Notebook::Notebook(Window *parent, bool _showButtons)
    : BaseWidget(parent)
    , parentWindow(parent)
    , addButton(this)
    , settingsButton(this)
    , userButton(this)
    , showButtons(_showButtons)
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

    closeConfirmDialog.setText("Are you sure you want to close this tab?");
    closeConfirmDialog.setIcon(QMessageBox::Icon::Question);
    closeConfirmDialog.setStandardButtons(QMessageBox::Yes | QMessageBox::Cancel);
    closeConfirmDialog.setDefaultButton(QMessageBox::Yes);

    this->scaleChangedEvent(this->getScale());

    // Window-wide hotkeys
    // CTRL+T: Create new split in selected notebook page
    CreateWindowShortcut(this, "CTRL+T", [this]() {
        if (this->selectedPage == nullptr) {
            return;
        }

        this->selectedPage->addChat(true);
    });
}

SplitContainer *Notebook::addNewPage(bool select)
{
    auto tab = new NotebookTab(this);
    auto page = new SplitContainer(this, tab);

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

    if (this->pages.empty()) {
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

SplitContainer *Notebook::getOrAddSelectedPage()
{
    if (selectedPage == nullptr) {
        this->addNewPage(true);
    }

    return selectedPage;
}

SplitContainer *Notebook::getSelectedPage()
{
    return selectedPage;
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
        rect.setHeight((int)(this->getScale() * 24));

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

SplitContainer *Notebook::tabAt(int index)
{
    return this->pages[index];
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
    float scale = this->getScale();
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
            //            y += 20;
            i->getTab()->moveAnimated(QPoint(0, y), animated);
            x = i->getTab()->width();
        } else {
            i->getTab()->moveAnimated(QPoint(x, y), animated);
            x += i->getTab()->width();
        }

        //        x -= (int)(8 * scale);
        x += 1;

        first = false;
    }

    //    x += (int)(8 * scale);
    //    x += 1;

    this->addButton.move(x, y);

    y -= 1;

    for (auto &i : this->pages) {
        i->getTab()->raise();
    }

    this->addButton.raise();

    if (this->selectedPage != nullptr) {
        this->selectedPage->move(0, y + tabHeight);
        this->selectedPage->resize(width(), height() - y - tabHeight);
        this->selectedPage->raise();
    }
}

void Notebook::resizeEvent(QResizeEvent *)
{
    this->performLayout(false);
}

void Notebook::scaleChangedEvent(float)
{
    float h = 24 * this->getScale();

    this->settingsButton.setFixedSize(h, h);
    this->userButton.setFixedSize(h, h);
    this->addButton.setFixedSize(h, h);

    for (auto &i : this->pages) {
        i->getTab()->updateSize();
    }
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
    QTimer::singleShot(80, [this] { this->addNewPage(true); });
}

}  // namespace widgets
}  // namespace chatterino
