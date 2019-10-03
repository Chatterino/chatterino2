#include "widgets/Notebook.hpp"

#include "Application.hpp"
#include "debug/Log.hpp"
#include "singletons/Settings.hpp"
#include "singletons/Theme.hpp"
#include "singletons/WindowManager.hpp"
#include "util/InitUpdateButton.hpp"
#include "util/Shortcut.hpp"
#include "widgets/Window.hpp"
#include "widgets/dialogs/SettingsDialog.hpp"
#include "widgets/helper/NotebookButton.hpp"
#include "widgets/helper/NotebookTab.hpp"
#include "widgets/splits/Split.hpp"
#include "widgets/splits/SplitContainer.hpp"

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
    , addButton_(new NotebookButton(this))
{
    this->addButton_->setIcon(NotebookButton::Icon::Plus);

    this->addButton_->setHidden(true);
}

NotebookTab *Notebook::addPage(QWidget *page, QString title, bool select)
{
    // Queue up save because: Tab added
    getApp()->windows->queueSave();

    auto *tab = new NotebookTab(this);
    tab->page = page;

    tab->setCustomTitle(title);

    Item item;
    item.page = page;
    item.tab = tab;

    this->items_.append(item);

    page->hide();
    page->setParent(this);

    if (select || this->items_.count() == 1)
    {
        this->select(page);
    }

    this->performLayout();

    tab->show();

    return tab;
}

void Notebook::removePage(QWidget *page)
{
    // Queue up save because: Tab removed
    getApp()->windows->queueSave();

    for (int i = 0; i < this->items_.count(); i++)
    {
        if (this->items_[i].page == page)
        {
            if (this->items_.count() == 1)
            {
                this->select(nullptr);
            }
            else if (i == this->items_.count() - 1)
            {
                this->select(this->items_[i - 1].page);
            }
            else
            {
                this->select(this->items_[i + 1].page);
            }

            this->items_[i].page->deleteLater();
            this->items_[i].tab->deleteLater();

            //    if (this->items.empty()) {
            //        this->addNewPage();
            //    }

            this->items_.removeAt(i);
            break;
        }
    }

    this->performLayout(true);
}

void Notebook::removeCurrentPage()
{
    if (this->selectedPage_ != nullptr)
    {
        this->removePage(this->selectedPage_);
    }
}

int Notebook::indexOf(QWidget *page) const
{
    for (int i = 0; i < this->items_.count(); i++)
    {
        if (this->items_[i].page == page)
        {
            return i;
        }
    }

    return -1;
}

void Notebook::select(QWidget *page)
{
    if (page == this->selectedPage_)
    {
        return;
    }

    if (page != nullptr)
    {
        page->setHidden(false);

        assert(this->containsPage(page));
        Item &item = this->findItem(page);

        item.tab->setSelected(true);
        item.tab->raise();

        if (item.selectedWidget == nullptr)
        {
            item.page->setFocus();
        }
        else
        {
            if (containsChild(page, item.selectedWidget))
            {
                qDebug() << item.selectedWidget;
                item.selectedWidget->setFocus(Qt::MouseFocusReason);
            }
            else
            {
                qDebug()
                    << "Notebook: selected child of page doesn't exist anymore";
            }
        }
    }

    if (this->selectedPage_ != nullptr)
    {
        this->selectedPage_->setHidden(true);

        Item &item = this->findItem(selectedPage_);
        item.tab->setSelected(false);

        //        for (auto split : this->selectedPage->getSplits()) {
        //            split->updateLastReadMessage();
        //        }

        item.selectedWidget = this->selectedPage_->focusWidget();
    }

    this->selectedPage_ = page;

    this->performLayout();
}

bool Notebook::containsPage(QWidget *page)
{
    return std::any_of(this->items_.begin(), this->items_.end(),
                       [page](const auto &item) { return item.page == page; });
}

Notebook::Item &Notebook::findItem(QWidget *page)
{
    auto it =
        std::find_if(this->items_.begin(), this->items_.end(),
                     [page](const auto &item) { return page == item.page; });
    assert(it != this->items_.end());
    return *it;
}

bool Notebook::containsChild(const QObject *obj, const QObject *child)
{
    return std::any_of(obj->children().begin(), obj->children().end(),
                       [child](const QObject *o) {
                           if (o == child)
                           {
                               return true;
                           }

                           return containsChild(o, child);
                       });
}

void Notebook::selectIndex(int index)
{
    if (index < 0 || this->items_.count() <= index)
    {
        return;
    }

    this->select(this->items_[index].page);
}

void Notebook::selectNextTab()
{
    if (this->items_.size() <= 1)
    {
        return;
    }

    auto index =
        (this->indexOf(this->selectedPage_) + 1) % this->items_.count();

    this->select(this->items_[index].page);
}

void Notebook::selectPreviousTab()
{
    if (this->items_.size() <= 1)
    {
        return;
    }

    int index = this->indexOf(this->selectedPage_) - 1;

    if (index < 0)
    {
        index += this->items_.count();
    }

    this->select(this->items_[index].page);
}

void Notebook::selectLastTab()
{
    const auto size = this->items_.size();
    if (size <= 1)
    {
        return;
    }

    this->select(this->items_[size - 1].page);
}

int Notebook::getPageCount() const
{
    return this->items_.count();
}

QWidget *Notebook::getPageAt(int index) const
{
    return this->items_[index].page;
}

int Notebook::getSelectedIndex() const
{
    return this->indexOf(this->selectedPage_);
}

QWidget *Notebook::getSelectedPage() const
{
    return this->selectedPage_;
}

QWidget *Notebook::tabAt(QPoint point, int &index, int maxWidth)
{
    auto i = 0;

    for (auto &item : this->items_)
    {
        auto rect = item.tab->getDesiredRect();
        rect.setHeight(int(this->scale() * 24));

        rect.setWidth(std::min(maxWidth, rect.width()));

        if (rect.contains(point))
        {
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
    // Queue up save because: Tab rearranged
    getApp()->windows->queueSave();

    this->items_.move(this->indexOf(page), index);

    this->performLayout(true);
}

bool Notebook::getAllowUserTabManagement() const
{
    return this->allowUserTabManagement_;
}

void Notebook::setAllowUserTabManagement(bool value)
{
    this->allowUserTabManagement_ = value;
}

bool Notebook::getShowAddButton() const
{
    return this->showAddButton_;
}

void Notebook::setShowAddButton(bool value)
{
    this->showAddButton_ = value;

    this->addButton_->setHidden(!value);
}

void Notebook::scaleChangedEvent(float scale)
{
    float h = (NOTEBOOK_TAB_HEIGHT - 1) * this->scale();

    this->addButton_->setFixedSize(h, h);

    for (auto &i : this->items_)
    {
        i.tab->updateSize();
    }
}

void Notebook::resizeEvent(QResizeEvent *)
{
    this->performLayout();
}

void Notebook::performLayout(bool animated)
{
    const auto left = int(2 * this->scale());
    const auto scale = this->scale();
    const auto tabHeight = int(NOTEBOOK_TAB_HEIGHT * scale);
    const auto addButtonWidth = this->showAddButton_ ? tabHeight : 0;

    auto x = left;
    auto y = 0;

    // set size of custom buttons (settings, user, ...)
    for (auto *btn : this->customButtons_)
    {
        if (!btn->isVisible())
        {
            continue;
        }

        btn->setFixedSize(tabHeight, tabHeight - 1);
        btn->move(x, 0);
        x += tabHeight;
    }

    // layout tabs
    /// Notebook tabs need to know if they are in the last row.
    auto firstInBottomRow =
        this->items_.size() ? &this->items_.front() : nullptr;

    for (auto &item : this->items_)
    {
        /// Break line if element doesn't fit.
        auto isFirst = &item == &this->items_.front();
        auto isLast = &item == &this->items_.back();

        auto fitsInLine =
            ((isLast ? addButtonWidth : 0) + x + item.tab->width()) <= width();

        if (!isFirst && !fitsInLine)
        {
            y += item.tab->height();
            x = left;
            firstInBottomRow = &item;
        }

        /// Layout tab
        item.tab->moveAnimated(QPoint(x, y), animated);
        x += item.tab->width() + std::max<int>(1, int(scale * 1));
    }

    /// Update which tabs are in the last row
    auto inLastRow = false;
    for (const auto &item : this->items_)
    {
        if (&item == firstInBottomRow)
        {
            inLastRow = true;
        }
        item.tab->setInLastRow(inLastRow);
    }

    // move misc buttons
    if (this->showAddButton_)
    {
        this->addButton_->move(x, y);
    }

    if (this->lineY_ != y + tabHeight)
    {
        this->lineY_ = y + tabHeight;
        this->update();
    }

    /// Increment for the line at the bottom
    y += int(2 * scale);

    // raise elements
    for (auto &i : this->items_)
    {
        i.tab->raise();
    }

    if (this->showAddButton_)
    {
        this->addButton_->raise();
    }

    // set page bounds
    if (this->selectedPage_ != nullptr)
    {
        this->selectedPage_->move(0, y + tabHeight);
        this->selectedPage_->resize(width(), height() - y - tabHeight);
        this->selectedPage_->raise();
    }
}

void Notebook::paintEvent(QPaintEvent *event)
{
    BaseWidget::paintEvent(event);

    QPainter painter(this);
    painter.fillRect(0, this->lineY_, this->width(), int(2 * this->scale()),
                     this->theme->tabs.bottomLine);
}

NotebookButton *Notebook::getAddButton()
{
    return this->addButton_;
}

NotebookButton *Notebook::addCustomButton()
{
    NotebookButton *btn = new NotebookButton(this);

    this->customButtons_.push_back(btn);

    this->performLayout();
    return btn;
}

NotebookTab *Notebook::getTabFromPage(QWidget *page)
{
    for (auto &it : this->items_)
    {
        if (it.page == page)
        {
            return it.tab;
        }
    }

    return nullptr;
}

SplitNotebook::SplitNotebook(Window *parent)
    : Notebook(parent)
{
    this->connect(this->getAddButton(), &NotebookButton::leftClicked, [this]() {
        QTimer::singleShot(80, this, [this] { this->addPage(true); });
    });

    // add custom buttons if they are not in the parent window frame
    if (!parent->hasCustomWindowFrame())
    {
        this->addCustomButtons();
    }
}

void SplitNotebook::addCustomButtons()
{
    // settings
    auto settingsBtn = this->addCustomButton();

    settingsBtn->setVisible(!getSettings()->hidePreferencesButton.getValue());

    getSettings()->hidePreferencesButton.connect(
        [settingsBtn](bool hide, auto) { settingsBtn->setVisible(!hide); },
        this->connections_);

    settingsBtn->setIcon(NotebookButton::Settings);

    QObject::connect(settingsBtn, &NotebookButton::leftClicked,
                     [] { getApp()->windows->showSettingsDialog(); });

    // account
    auto userBtn = this->addCustomButton();
    userBtn->setVisible(!getSettings()->hideUserButton.getValue());
    getSettings()->hideUserButton.connect(
        [userBtn](bool hide, auto) { userBtn->setVisible(!hide); },
        this->connections_);

    userBtn->setIcon(NotebookButton::User);
    QObject::connect(userBtn, &NotebookButton::leftClicked, [this, userBtn] {
        getApp()->windows->showAccountSelectPopup(
            this->mapToGlobal(userBtn->rect().bottomRight()));
    });

    // updates
    auto updateBtn = this->addCustomButton();

    //initUpdateButton(*updateBtn, this->signalHolder_);
}

SplitContainer *SplitNotebook::addPage(bool select)
{
    auto container = new SplitContainer(this);
    auto tab = Notebook::addPage(container, QString(), select);
    container->setTab(tab);
    tab->setParent(this);
    tab->show();
    return container;
}

SplitContainer *SplitNotebook::getOrAddSelectedPage()
{
    auto *selectedPage = this->getSelectedPage();

    return selectedPage != nullptr ? (SplitContainer *)selectedPage
                                   : this->addPage();
}

void SplitNotebook::select(QWidget *page)
{
    if (auto selectedPage = this->getSelectedPage())
    {
        if (auto splitContainer = dynamic_cast<SplitContainer *>(selectedPage))
        {
            for (auto split : splitContainer->getSplits())
            {
                split->updateLastReadMessage();
            }
        }
    }
    this->Notebook::select(page);
}

}  // namespace chatterino
