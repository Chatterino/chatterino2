#include "widgets/Notebook.hpp"

#include "Application.hpp"
#include "debug/Log.hpp"
#include "singletons/ThemeManager.hpp"
#include "singletons/WindowManager.hpp"
#include "widgets/helper/NotebookButton.hpp"
#include "widgets/helper/NotebookTab.hpp"
#include "widgets/helper/Shortcut.hpp"
#include "widgets/dialogs/SettingsDialog.hpp"
#include "widgets/splits/SplitContainer.hpp"
#include "widgets/Window.hpp"

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

Notebook::Notebook(QWidget *parent)
    : BaseWidget(parent)
    , addButton(this)
{
    this->addButton.setHidden(true);

    auto *shortcut_next = new QShortcut(QKeySequence("Ctrl+Tab"), this);
    QObject::connect(shortcut_next, &QShortcut::activated, [this] { this->selectNextTab(); });

    auto *shortcut_prev = new QShortcut(QKeySequence("Ctrl+Shift+Tab"), this);
    QObject::connect(shortcut_prev, &QShortcut::activated, [this] { this->selectPreviousTab(); });
}

NotebookTab *Notebook::addPage(QWidget *page, QString title, bool select)
{
    auto *tab = new NotebookTab(this);
    tab->page = page;

    tab->setCustomTitle(title);

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

    tab->show();

    return tab;
}

void Notebook::removePage(QWidget *page)
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

    this->performLayout(true);
}

void Notebook::removeCurrentPage()
{
    if (this->selectedPage != nullptr) {
        this->removePage(this->selectedPage);
    }
}

int Notebook::indexOf(QWidget *page) const
{
    for (int i = 0; i < this->items.count(); i++) {
        if (this->items[i].page == page) {
            return i;
        }
    }

    return -1;
}

void Notebook::select(QWidget *page)
{
    if (page == this->selectedPage) {
        return;
    }

    if (page != nullptr) {
        page->setHidden(false);

        assert(this->containsPage(page));
        Item &item = this->findItem(page);

        item.tab->setSelected(true);
        item.tab->raise();

        if (item.selectedWidget == nullptr) {
            item.page->setFocus();
        } else {
            if (containsChild(page, item.selectedWidget)) {
                qDebug() << item.selectedWidget;
                item.selectedWidget->setFocus(Qt::MouseFocusReason);
            } else {
                qDebug() << "Notebook: selected child of page doesn't exist anymore";
            }
        }
    }

    if (this->selectedPage != nullptr) {
        this->selectedPage->setHidden(true);

        Item &item = this->findItem(selectedPage);
        item.tab->setSelected(false);

        //        for (auto split : this->selectedPage->getSplits()) {
        //            split->updateLastReadMessage();
        //        }

        item.selectedWidget = this->selectedPage->focusWidget();
    }

    this->selectedPage = page;

    this->performLayout();
}

bool Notebook::containsPage(QWidget *page)
{
    return std::any_of(this->items.begin(), this->items.end(),
                       [page](const auto &item) { return item.page == page; });
}

Notebook::Item &Notebook::findItem(QWidget *page)
{
    auto it = std::find_if(this->items.begin(), this->items.end(),
                           [page](const auto &item) { return page == item.page; });
    assert(it != this->items.end());
    return *it;
}

bool Notebook::containsChild(const QObject *obj, const QObject *child)
{
    return std::any_of(obj->children().begin(), obj->children().end(), [child](const QObject *o) {
        if (o == child) {
            return true;
        }

        return containsChild(o, child);
    });
}

void Notebook::selectIndex(int index)
{
    if (index < 0 || this->items.count() <= index) {
        return;
    }

    this->select(this->items[index].page);
}

void Notebook::selectNextTab()
{
    if (this->items.size() <= 1) {
        return;
    }

    int index = (this->indexOf(this->selectedPage) + 1) % this->items.count();

    this->select(this->items[index].page);
}

void Notebook::selectPreviousTab()
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

int Notebook::getPageCount() const
{
    return this->items.count();
}

QWidget *Notebook::getPageAt(int index) const
{
    return this->items[index].page;
}

int Notebook::getSelectedIndex() const
{
    return this->indexOf(this->selectedPage);
}

QWidget *Notebook::getSelectedPage() const
{
    return this->selectedPage;
}

QWidget *Notebook::tabAt(QPoint point, int &index, int maxWidth)
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

void Notebook::rearrangePage(QWidget *page, int index)
{
    this->items.move(this->indexOf(page), index);

    this->performLayout(true);
}

bool Notebook::getAllowUserTabManagement() const
{
    return this->allowUserTabManagement;
}

void Notebook::setAllowUserTabManagement(bool value)
{
    this->allowUserTabManagement = value;
}

bool Notebook::getShowAddButton() const
{
    return this->showAddButton;
}

void Notebook::setShowAddButton(bool value)
{
    this->showAddButton = value;

    this->addButton.setHidden(!value);
}

void Notebook::scaleChangedEvent(float scale)
{
    float h = NOTEBOOK_TAB_HEIGHT * this->getScale();

    this->addButton.setFixedSize(h, h);

    for (auto &i : this->items) {
        i.tab->updateSize();
    }
}

void Notebook::resizeEvent(QResizeEvent *)
{
    this->performLayout();
}

void Notebook::performLayout(bool animated)
{
    auto app = getApp();

    int xStart = int(2 * this->getScale());

    int x = xStart, y = 0;
    float scale = this->getScale();

    int h = int(NOTEBOOK_TAB_HEIGHT * this->getScale());

    for (auto *btn : this->customButtons) {
        if (!btn->isVisible()) {
            continue;
        }

        btn->setFixedSize(h, h);
        btn->move(x, 0);
        x += h;
    }

    int tabHeight = static_cast<int>(NOTEBOOK_TAB_HEIGHT * scale);
    bool first = true;

    for (auto i = this->items.begin(); i != this->items.end(); i++) {
        bool wrap =
            !first && (((i + 1 == this->items.end() && this->showAddButton) ? tabHeight : 0) + x +
                       i->tab->width()) > width();

        if (wrap) {
            y += i->tab->height();
            i->tab->moveAnimated(QPoint(xStart, y), animated);
            x = i->tab->width() + xStart;
        } else {
            i->tab->moveAnimated(QPoint(x, y), animated);
            x += i->tab->width();
        }

        x += int(scale * 1);

        first = false;
    }

    if (this->showAddButton) {
        this->addButton.move(x, y);
    }

    if (this->lineY != y + tabHeight) {
        this->lineY = y + tabHeight;
        this->update();
    }

    y += int(3 * scale);

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

void Notebook::paintEvent(QPaintEvent *event)
{
    BaseWidget::paintEvent(event);

    QPainter painter(this);
    painter.fillRect(0, this->lineY, this->width(), (int)(3 * this->getScale()),
                     this->themeManager->tabs.bottomLine);
}

NotebookButton *Notebook::getAddButton()
{
    return &this->addButton;
}

NotebookButton *Notebook::addCustomButton()
{
    NotebookButton *btn = new NotebookButton(this);

    this->customButtons.push_back(btn);

    this->performLayout();
    return btn;
}

NotebookTab *Notebook::getTabFromPage(QWidget *page)
{
    for (auto &it : this->items) {
        if (it.page == page) {
            return it.tab;
        }
    }

    return nullptr;
}

SplitNotebook::SplitNotebook(Window *parent)
    : Notebook(parent)
{
    auto app = getApp();

    this->connect(this->getAddButton(), &NotebookButton::clicked,
                  [this]() { QTimer::singleShot(80, this, [this] { this->addPage(true); }); });

    bool customFrame = parent->hasCustomWindowFrame();

    if (!customFrame) {
        auto *settingsBtn = this->addCustomButton();
        auto *userBtn = this->addCustomButton();

        settingsBtn->setVisible(!app->settings->hidePreferencesButton.getValue());
        userBtn->setVisible(!app->settings->hideUserButton.getValue());

        app->settings->hidePreferencesButton.connect(
            [this, settingsBtn](bool hide, auto) {
                settingsBtn->setVisible(!hide);
                this->performLayout(true);
            },
            this->uniqueConnections);

        app->settings->hideUserButton.connect(
            [this, userBtn](bool hide, auto) {
                userBtn->setVisible(!hide);
                this->performLayout(true);
            },
            this->uniqueConnections);

        settingsBtn->icon = NotebookButton::IconSettings;
        userBtn->icon = NotebookButton::IconUser;

        QObject::connect(settingsBtn, &NotebookButton::clicked,
                         [] { getApp()->windows->showSettingsDialog(); });

        QObject::connect(userBtn, &NotebookButton::clicked, [this, userBtn] {
            getApp()->windows->showAccountSelectPopup(
                this->mapToGlobal(userBtn->rect().bottomRight()));
        });
    }
}

SplitContainer *SplitNotebook::addPage(bool select)
{
    SplitContainer *container = new SplitContainer(this);
    auto *tab = Notebook::addPage(container, QString(), select);
    container->setTab(tab);
    tab->setParent(this);
    tab->show();
    return container;
}

SplitContainer *SplitNotebook::getOrAddSelectedPage()
{
    auto *selectedPage = this->getSelectedPage();

    return selectedPage != nullptr ? (SplitContainer *)selectedPage : this->addPage();
}

}  // namespace chatterino
