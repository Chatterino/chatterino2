#include "widgets/notebook.hpp"

#include "application.hpp"
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

Notebook2::Notebook2(QWidget *parent)
    : BaseWidget(parent)
    , addButton(this)
{
    this->addButton.setHidden(true);

    auto *shortcut_next = new QShortcut(QKeySequence("Ctrl+Tab"), this);
    QObject::connect(shortcut_next, &QShortcut::activated, [this] { this->selectNextTab(); });

    auto *shortcut_prev = new QShortcut(QKeySequence("Ctrl+Shift+Tab"), this);
    QObject::connect(shortcut_prev, &QShortcut::activated, [this] { this->selectPreviousTab(); });
}

NotebookTab2 *Notebook2::addPage(QWidget *page, QString title, bool select)
{
    auto *tab = new NotebookTab2(this);
    tab->page = page;

    if (!title.isEmpty()) {
        tab->setTitle(title);
        tab->useDefaultTitle = false;
    }

    Item item;
    item.page = page;
    item.tab = tab;

    this->items.append(item);

    page->hide();
    page->setParent(this);

    if (select || this->items.count() == 1) {
        this->select(page);
    }

    this->performLayout();

    return tab;
}

void Notebook2::removePage(QWidget *page)
{
    for (int i = 0; i < this->items.count(); i++) {
        if (this->items[i].page == page) {
            if (this->items.count() == 1) {
                this->select(nullptr);
            } else if (i == this->items.count() - 1) {
                this->select(this->items[i - 1].page);
            } else {
                this->select(this->items[i + 1].page);
            }

            this->items[i].page->deleteLater();
            this->items[i].tab->deleteLater();

            //    if (this->items.empty()) {
            //        this->addNewPage();
            //    }

            this->items.removeAt(i);
            break;
        }
    }
}

void Notebook2::removeCurrentPage()
{
    if (this->selectedPage != nullptr) {
        this->removePage(this->selectedPage);
    }
}

int Notebook2::indexOf(QWidget *page) const
{
    for (int i = 0; i < this->items.count(); i++) {
        if (this->items[i].page == page) {
            return i;
        }
    }

    return -1;
}

void Notebook2::select(QWidget *page)
{
    if (page == this->selectedPage) {
        return;
    }

    if (page != nullptr) {
        page->setHidden(false);

        NotebookTab2 *tab = this->getTabFromPage(page);
        tab->setSelected(true);
        tab->raise();
    }

    if (this->selectedPage != nullptr) {
        this->selectedPage->setHidden(true);

        NotebookTab2 *tab = this->getTabFromPage(selectedPage);
        tab->setSelected(false);

        //        for (auto split : this->selectedPage->getSplits()) {
        //            split->updateLastReadMessage();
        //        }
    }

    this->selectedPage = page;

    this->performLayout();
}

void Notebook2::selectIndex(int index)
{
    if (index < 0 || this->items.count() <= index) {
        return;
    }

    this->select(this->items[index].page);
}

void Notebook2::selectNextTab()
{
    if (this->items.size() <= 1) {
        return;
    }

    int index = (this->indexOf(this->selectedPage) + 1) % this->items.count();

    this->select(this->items[index].page);
}

void Notebook2::selectPreviousTab()
{
    if (this->items.size() <= 1) {
        return;
    }

    int index = this->indexOf(this->selectedPage) - 1;

    if (index < 0) {
        index += this->items.count();
    }

    this->select(this->items[index].page);
}

int Notebook2::getPageCount() const
{
    return this->items.count();
}

int Notebook2::getSelectedIndex() const
{
    return this->indexOf(this->selectedPage);
}

QWidget *Notebook2::getSelectedPage() const
{
    return this->selectedPage;
}

QWidget *Notebook2::tabAt(QPoint point, int &index, int maxWidth)
{
    int i = 0;

    for (auto &item : this->items) {
        QRect rect = item.tab->getDesiredRect();
        rect.setHeight((int)(this->getScale() * 24));

        rect.setWidth(std::min(maxWidth, rect.width()));

        if (rect.contains(point)) {
            index = i;
            return item.page;
        }

        i++;
    }

    index = -1;
    return nullptr;
}

void Notebook2::rearrangePage(QWidget *page, int index)
{
    this->items.move(this->indexOf(page), index);

    this->performLayout();
}

bool Notebook2::getAllowUserTabManagement() const
{
    return this->allowUserTabManagement;
}

void Notebook2::setAllowUserTabManagement(bool value)
{
    this->allowUserTabManagement = value;
}

bool Notebook2::getShowAddButton() const
{
    return this->showAddButton;
}

void Notebook2::setShowAddButton(bool value)
{
    this->showAddButton = value;

    this->addButton.setHidden(!value);
}

void Notebook2::scaleChangedEvent(float scale)
{
    //    float h = 24 * this->getScale();

    //    this->settingsButton.setFixedSize(h, h);
    //    this->userButton.setFixedSize(h, h);
    //    this->addButton.setFixedSize(h, h);

    for (auto &i : this->items) {
        i.tab->updateSize();
    }
}

void Notebook2::resizeEvent(QResizeEvent *)
{
    this->performLayout();
}

void Notebook2::performLayout(bool animated)
{
    auto app = getApp();

    int xStart = (int)(2 * this->getScale());

    int x = xStart, y = 0;
    float scale = this->getScale();

    //    bool customFrame = this->parentWindow->hasCustomWindowFrame();

    //    bool customFrame = false;

    //    if (!this->showButtons || app->settings->hidePreferencesButton || customFrame) {
    //        this->settingsButton.hide();
    //    } else {
    //        this->settingsButton.show();
    //        x += settingsButton.width();
    //    }
    //    if (!this->showButtons || app->settings->hideUserButton || customFrame) {
    //        this->userButton.hide();
    //    } else {
    //        this->userButton.move(x, 0);
    //        this->userButton.show();
    //        x += userButton.width();
    //    }

    //    if (customFrame || !this->showButtons ||
    //        (app->settings->hideUserButton && app->settings->hidePreferencesButton)) {
    //        x += (int)(scale * 2);
    //    }

    int tabHeight = static_cast<int>(24 * scale);
    bool first = true;

    for (auto i = this->items.begin(); i != this->items.end(); i++) {
        if (!first &&
            (i == this->items.end() && this->showAddButton ? tabHeight : 0) + x + i->tab->width() >
                width())  //
        {
            y += i->tab->height();
            //            y += 20;
            i->tab->moveAnimated(QPoint(xStart, y), animated);
            x = i->tab->width() + xStart;
        } else {
            i->tab->moveAnimated(QPoint(x, y), animated);
            x += i->tab->width();
        }

        x += 1;

        first = false;
    }

    if (this->showAddButton) {
        this->addButton.move(x, y);
    }

    if (this->lineY != y + tabHeight) {
        this->lineY = y + tabHeight;
        this->update();
    }

    y += (int)(1 * scale);

    for (auto &i : this->items) {
        i.tab->raise();
    }

    if (this->showAddButton) {
        this->addButton.raise();
    }

    if (this->selectedPage != nullptr) {
        this->selectedPage->move(0, y + tabHeight);
        this->selectedPage->resize(width(), height() - y - tabHeight);
        this->selectedPage->raise();
    }
}

void Notebook2::paintEvent(QPaintEvent *event)
{
    BaseWidget::paintEvent(event);

    QPainter painter(this);
    painter.fillRect(0, this->lineY, this->width(), (int)(1 * this->getScale()),
                     this->themeManager->tabs.bottomLine);
}

NotebookTab2 *Notebook2::getTabFromPage(QWidget *page)
{
    for (auto &it : this->items) {
        if (it.page == page) {
            return it.tab;
        }
    }

    return nullptr;
}

// Notebook2::OLD NOTEBOOK
Notebook::Notebook(Window *parent, bool _showButtons)
    : BaseWidget(parent)
    , parentWindow(parent)
    , addButton(this)
    , settingsButton(this)
    , userButton(this)
    , showButtons(_showButtons)
    , closeConfirmDialog(this)
{
    auto app = getApp();

    this->connect(&this->settingsButton, SIGNAL(clicked()), this, SLOT(settingsButtonClicked()));
    this->connect(&this->userButton, SIGNAL(clicked()), this, SLOT(usersButtonClicked()));
    this->connect(&this->addButton, SIGNAL(clicked()), this, SLOT(addPageButtonClicked()));

    this->settingsButton.icon = NotebookButton::IconSettings;

    this->userButton.move(24, 0);
    this->userButton.icon = NotebookButton::IconUser;

    app->settings->hidePreferencesButton.connectSimple([this](auto) { this->performLayout(); });
    app->settings->hideUserButton.connectSimple([this](auto) { this->performLayout(); });

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
    auto app = getApp();

    int x = 0, y = 0;
    float scale = this->getScale();
    bool customFrame = this->parentWindow->hasCustomWindowFrame();

    if (!this->showButtons || app->settings->hidePreferencesButton || customFrame) {
        this->settingsButton.hide();
    } else {
        this->settingsButton.show();
        x += settingsButton.width();
    }
    if (!this->showButtons || app->settings->hideUserButton || customFrame) {
        this->userButton.hide();
    } else {
        this->userButton.move(x, 0);
        this->userButton.show();
        x += userButton.width();
    }

    if (customFrame || !this->showButtons ||
        (app->settings->hideUserButton && app->settings->hidePreferencesButton)) {
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
    auto app = getApp();
    app->windows->showSettingsDialog();
}

void Notebook::usersButtonClicked()
{
    auto app = getApp();
    app->windows->showAccountSelectPopup(
        this->mapToGlobal(this->userButton.rect().bottomRight()));
}

void Notebook::addPageButtonClicked()
{
    QTimer::singleShot(80, [this] { this->addNewPage(true); });
}

}  // namespace widgets
}  // namespace chatterino
