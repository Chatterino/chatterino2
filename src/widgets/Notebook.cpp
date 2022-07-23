#include "widgets/Notebook.hpp"

#include "Application.hpp"
#include "common/QLogging.hpp"
#include "singletons/Settings.hpp"
#include "singletons/Theme.hpp"
#include "singletons/WindowManager.hpp"
#include "util/InitUpdateButton.hpp"
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
#include <QStandardPaths>
#include <QUuid>
#include <QWidget>
#include <boost/foreach.hpp>

namespace chatterino {

Notebook::Notebook(QWidget *parent)
    : BaseWidget(parent)
    , menu_(this)
    , addButton_(new NotebookButton(this))
{
    this->addButton_->setIcon(NotebookButton::Icon::Plus);

    this->addButton_->setHidden(true);

    this->lockNotebookLayoutAction_ = new QAction("Lock Tab Layout", this);

    // Load lock notebook layout state from settings
    this->setLockNotebookLayout(getSettings()->lockNotebookLayout.getValue());

    this->lockNotebookLayoutAction_->setCheckable(true);
    this->lockNotebookLayoutAction_->setChecked(this->lockNotebookLayout_);

    // Update lockNotebookLayout_ value anytime the user changes the checkbox state
    QObject::connect(this->lockNotebookLayoutAction_, &QAction::triggered,
                     [this](bool value) {
                         this->setLockNotebookLayout(value);
                     });

    this->addNotebookActionsToMenu(&this->menu_);

    // Manually resize the add button so the initial paint uses the correct
    // width when computing the maximum width occupied per column in vertical
    // tab rendering.
    this->resizeAddButton();
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

void Notebook::select(QWidget *page, bool focusPage)
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

        if (focusPage)
        {
            if (item.selectedWidget == nullptr)
            {
                item.page->setFocus();
            }
            else
            {
                if (containsChild(page, item.selectedWidget))
                {
                    item.selectedWidget->setFocus(Qt::MouseFocusReason);
                }
                else
                {
                    qCDebug(chatterinoWidget) << "Notebook: selected child of "
                                                 "page doesn't exist anymore";
                }
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
                       [page](const auto &item) {
                           return item.page == page;
                       });
}

Notebook::Item &Notebook::findItem(QWidget *page)
{
    auto it = std::find_if(this->items_.begin(), this->items_.end(),
                           [page](const auto &item) {
                               return page == item.page;
                           });
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

void Notebook::selectIndex(int index, bool focusPage)
{
    if (index < 0 || this->items_.count() <= index)
    {
        return;
    }

    this->select(this->items_[index].page, focusPage);
}

void Notebook::selectNextTab(bool focusPage)
{
    if (this->items_.size() <= 1)
    {
        return;
    }

    auto index =
        (this->indexOf(this->selectedPage_) + 1) % this->items_.count();

    this->select(this->items_[index].page, focusPage);
}

void Notebook::selectPreviousTab(bool focusPage)
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

    this->select(this->items_[index].page, focusPage);
}

void Notebook::selectLastTab(bool focusPage)
{
    const auto size = this->items_.size();
    if (size <= 1)
    {
        return;
    }

    this->select(this->items_[size - 1].page, focusPage);
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
    if (this->isNotebookLayoutLocked())
    {
        return;
    }

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

bool Notebook::getShowTabs() const
{
    return this->showTabs_;
}

void Notebook::setShowTabs(bool value)
{
    this->showTabs_ = value;

    this->performLayout();
    for (auto &item : this->items_)
    {
        item.tab->setHidden(!value);
    }

    this->setShowAddButton(value);

    // show a popup upon hiding tabs
    if (!value && getSettings()->informOnTabVisibilityToggle.getValue())
    {
        QMessageBox msgBox(this->window());
        msgBox.window()->setWindowTitle("Chatterino - hidden tabs");
        msgBox.setText("You've just hidden your tabs.");
        msgBox.setInformativeText(
            "You can toggle tabs by using the keyboard shortcut (Ctrl+U by "
            "default) or right-clicking the tab area and selecting \"Toggle "
            "visibility of tabs\".");
        msgBox.addButton(QMessageBox::Ok);
        auto *dsaButton =
            msgBox.addButton("Don't show again", QMessageBox::YesRole);

        msgBox.setDefaultButton(QMessageBox::Ok);

        msgBox.exec();

        if (msgBox.clickedButton() == dsaButton)
        {
            getSettings()->informOnTabVisibilityToggle.setValue(false);
        }
    }
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

void Notebook::resizeAddButton()
{
    float h = (NOTEBOOK_TAB_HEIGHT - 1) * this->scale();
    this->addButton_->setFixedSize(h, h);
}

void Notebook::scaleChangedEvent(float)
{
    this->resizeAddButton();
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
    const auto right = width();
    const auto bottom = height();
    const auto scale = this->scale();
    const auto tabHeight = int(NOTEBOOK_TAB_HEIGHT * scale);
    const auto minimumTabAreaSpace = int(tabHeight * 0.5);
    const auto addButtonWidth = this->showAddButton_ ? tabHeight : 0;
    const auto lineThickness = int(2 * scale);

    const auto buttonWidth = tabHeight;
    const auto buttonHeight = tabHeight - 1;

    if (this->tabLocation_ == NotebookTabLocation::Top)
    {
        auto x = left;
        auto y = 0;
        auto consumedButtonHeights = 0;

        // set size of custom buttons (settings, user, ...)
        for (auto *btn : this->customButtons_)
        {
            if (!btn->isVisible())
            {
                continue;
            }

            btn->setFixedSize(buttonWidth, buttonHeight);
            btn->move(x, 0);
            x += buttonWidth;

            consumedButtonHeights = tabHeight;
        }

        if (this->showTabs_)
        {
            // layout tabs
            /// Notebook tabs need to know if they are in the last row.
            auto firstInBottomRow =
                this->items_.size() ? &this->items_.front() : nullptr;

            for (auto &item : this->items_)
            {
                /// Break line if element doesn't fit.
                auto isFirst = &item == &this->items_.front();
                auto isLast = &item == &this->items_.back();

                auto fitsInLine = ((isLast ? addButtonWidth : 0) + x +
                                   item.tab->width()) <= width();

                if (!isFirst && !fitsInLine)
                {
                    y += item.tab->height();
                    x = left;
                    firstInBottomRow = &item;
                }

                /// Layout tab
                item.tab->growWidth(0);
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

            y += tabHeight;
        }

        y = std::max({y, consumedButtonHeights, minimumTabAreaSpace});

        if (this->lineOffset_ != y)
        {
            this->lineOffset_ = y;
            this->update();
        }

        /// Increment for the line at the bottom
        y += int(2 * scale);

        // set page bounds
        if (this->selectedPage_ != nullptr)
        {
            this->selectedPage_->move(0, y);
            this->selectedPage_->resize(width(), height() - y);
            this->selectedPage_->raise();
        }
    }
    else if (this->tabLocation_ == NotebookTabLocation::Left)
    {
        auto x = left;
        auto y = 0;

        // set size of custom buttons (settings, user, ...)
        for (auto *btn : this->customButtons_)
        {
            if (!btn->isVisible())
            {
                continue;
            }

            btn->setFixedSize(buttonWidth, buttonHeight);
            btn->move(x, y);
            x += buttonWidth;
        }

        if (this->visibleButtonCount() > 0)
            y = tabHeight;

        int totalButtonWidths = x;
        int top = y;
        x = left;

        // zneix: if we were to remove buttons when tabs are hidden
        // stuff below to "set page bounds" part should be in conditional statement
        int tabsPerColumn = (this->height() - top) / tabHeight;
        if (tabsPerColumn == 0)  // window hasn't properly rendered yet
        {
            return;
        }
        int count = this->items_.size() + (this->showAddButton_ ? 1 : 0);
        int columnCount = ceil((float)count / tabsPerColumn);

        // only add width of all the tabs if they are not hidden
        if (this->showTabs_)
        {
            for (int col = 0; col < columnCount; col++)
            {
                bool isLastColumn = col == columnCount - 1;
                auto largestWidth = 0;
                int tabStart = col * tabsPerColumn;
                int tabEnd =
                    std::min((col + 1) * tabsPerColumn, this->items_.size());

                for (int i = tabStart; i < tabEnd; i++)
                {
                    largestWidth = std::max(
                        this->items_.at(i).tab->normalTabWidth(), largestWidth);
                }

                if (isLastColumn && this->showAddButton_)
                {
                    largestWidth =
                        std::max(largestWidth, this->addButton_->width());
                }

                if (isLastColumn && largestWidth + x < totalButtonWidths)
                    largestWidth = totalButtonWidths - x;

                for (int i = tabStart; i < tabEnd; i++)
                {
                    auto item = this->items_.at(i);

                    /// Layout tab
                    item.tab->growWidth(largestWidth);
                    item.tab->moveAnimated(QPoint(x, y), animated);
                    y += tabHeight;
                }

                if (isLastColumn && this->showAddButton_)
                {
                    this->addButton_->move(x, y);
                }

                x += largestWidth + lineThickness;
                y = top;
            }
        }

        x = std::max({x, totalButtonWidths, minimumTabAreaSpace});

        if (this->lineOffset_ != x - lineThickness)
        {
            this->lineOffset_ = x - lineThickness;
            this->update();
        }

        // set page bounds
        if (this->selectedPage_ != nullptr)
        {
            this->selectedPage_->move(x, 0);
            this->selectedPage_->resize(width() - x, height());
            this->selectedPage_->raise();
        }
    }
    else if (this->tabLocation_ == NotebookTabLocation::Right)
    {
        auto x = right;
        auto y = 0;

        // set size of custom buttons (settings, user, ...)
        for (auto btnIt = this->customButtons_.rbegin();
             btnIt != this->customButtons_.rend(); ++btnIt)
        {
            auto btn = *btnIt;
            if (!btn->isVisible())
            {
                continue;
            }

            x -= buttonWidth;
            btn->setFixedSize(buttonWidth, buttonHeight);
            btn->move(x, y);
        }

        if (this->visibleButtonCount() > 0)
            y = tabHeight;

        int consumedButtonWidths = right - x;
        int top = y;
        x = right;

        // zneix: if we were to remove buttons when tabs are hidden
        // stuff below to "set page bounds" part should be in conditional statement
        int tabsPerColumn = (this->height() - top) / tabHeight;
        if (tabsPerColumn == 0)  // window hasn't properly rendered yet
        {
            return;
        }
        int count = this->items_.size() + (this->showAddButton_ ? 1 : 0);
        int columnCount = ceil((float)count / tabsPerColumn);

        // only add width of all the tabs if they are not hidden
        if (this->showTabs_)
        {
            for (int col = 0; col < columnCount; col++)
            {
                bool isLastColumn = col == columnCount - 1;
                auto largestWidth = 0;
                int tabStart = col * tabsPerColumn;
                int tabEnd =
                    std::min((col + 1) * tabsPerColumn, this->items_.size());

                for (int i = tabStart; i < tabEnd; i++)
                {
                    largestWidth = std::max(
                        this->items_.at(i).tab->normalTabWidth(), largestWidth);
                }

                if (isLastColumn && this->showAddButton_)
                {
                    largestWidth =
                        std::max(largestWidth, this->addButton_->width());
                }

                int distanceFromRight = width() - x;

                if (isLastColumn &&
                    largestWidth + distanceFromRight < consumedButtonWidths)
                    largestWidth = consumedButtonWidths - distanceFromRight;

                x -= largestWidth + lineThickness;

                for (int i = tabStart; i < tabEnd; i++)
                {
                    auto item = this->items_.at(i);

                    /// Layout tab
                    item.tab->growWidth(largestWidth);
                    item.tab->moveAnimated(QPoint(x, y), animated);
                    y += tabHeight;
                }

                if (isLastColumn && this->showAddButton_)
                {
                    this->addButton_->move(x, y);
                }

                y = top;
            }
        }

        // subtract another lineThickness to account for vertical divider
        x -= lineThickness;
        int consumedRightSpace =
            std::max({right - x, consumedButtonWidths, minimumTabAreaSpace});
        int tabsStart = right - consumedRightSpace;

        if (this->lineOffset_ != tabsStart)
        {
            this->lineOffset_ = tabsStart;
            this->update();
        }

        // set page bounds
        if (this->selectedPage_ != nullptr)
        {
            this->selectedPage_->move(0, 0);
            this->selectedPage_->resize(tabsStart, height());
            this->selectedPage_->raise();
        }
    }
    else if (this->tabLocation_ == NotebookTabLocation::Bottom)
    {
        auto x = left;
        auto y = bottom;
        auto consumedButtonHeights = 0;

        // set size of custom buttons (settings, user, ...)
        for (auto *btn : this->customButtons_)
        {
            if (!btn->isVisible())
            {
                continue;
            }

            // move upward to place button below location (x, y)
            y = bottom - tabHeight;

            btn->setFixedSize(buttonWidth, buttonHeight);
            btn->move(x, y);
            x += buttonWidth;

            consumedButtonHeights = tabHeight;
        }

        if (this->showTabs_)
        {
            // reset vertical position regardless
            y = bottom - tabHeight;

            // layout tabs
            /// Notebook tabs need to know if they are in the last row.
            auto firstInBottomRow =
                this->items_.size() ? &this->items_.front() : nullptr;

            for (auto &item : this->items_)
            {
                /// Break line if element doesn't fit.
                auto isFirst = &item == &this->items_.front();
                auto isLast = &item == &this->items_.back();

                auto fitsInLine = ((isLast ? addButtonWidth : 0) + x +
                                   item.tab->width()) <= width();

                if (!isFirst && !fitsInLine)
                {
                    y -= item.tab->height();
                    x = left;
                    firstInBottomRow = &item;
                }

                /// Layout tab
                item.tab->growWidth(0);
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
        }

        int consumedBottomSpace =
            std::max({bottom - y, consumedButtonHeights, minimumTabAreaSpace});
        int tabsStart = bottom - consumedBottomSpace;

        if (this->lineOffset_ != tabsStart)
        {
            this->lineOffset_ = tabsStart;
            this->update();
        }

        // set page bounds
        if (this->selectedPage_ != nullptr)
        {
            this->selectedPage_->move(0, 0);
            this->selectedPage_->resize(width(), tabsStart);
            this->selectedPage_->raise();
        }
    }

    if (this->showTabs_)
    {
        // raise elements
        for (auto &i : this->items_)
        {
            i.tab->raise();
        }

        if (this->showAddButton_)
        {
            this->addButton_->raise();
        }
    }
}

void Notebook::mousePressEvent(QMouseEvent *event)
{
    this->update();

    switch (event->button())
    {
        case Qt::RightButton: {
            this->menu_.popup(event->globalPos() + QPoint(0, 8));
        }
        break;
        default:;
    }
}

void Notebook::setTabLocation(NotebookTabLocation location)
{
    if (location != this->tabLocation_)
    {
        this->tabLocation_ = location;
        this->performLayout();
    }
}

void Notebook::paintEvent(QPaintEvent *event)
{
    BaseWidget::paintEvent(event);
    auto scale = this->scale();

    QPainter painter(this);
    if (this->tabLocation_ == NotebookTabLocation::Top ||
        this->tabLocation_ == NotebookTabLocation::Bottom)
    {
        /// horizontal line
        painter.fillRect(0, this->lineOffset_, this->width(), int(2 * scale),
                         this->theme->tabs.dividerLine);
    }
    else if (this->tabLocation_ == NotebookTabLocation::Left ||
             this->tabLocation_ == NotebookTabLocation::Right)
    {
        if (this->visibleButtonCount() > 0)
        {
            if (this->tabLocation_ == NotebookTabLocation::Left)
            {
                painter.fillRect(0, int(NOTEBOOK_TAB_HEIGHT * scale),
                                 this->lineOffset_, int(2 * scale),
                                 this->theme->tabs.dividerLine);
            }
            else
            {
                painter.fillRect(this->lineOffset_,
                                 int(NOTEBOOK_TAB_HEIGHT * scale),
                                 width() - this->lineOffset_, int(2 * scale),
                                 this->theme->tabs.dividerLine);
            }
        }

        /// vertical line
        painter.fillRect(this->lineOffset_, 0, int(2 * scale), this->height(),
                         this->theme->tabs.dividerLine);
    }
}

bool Notebook::isNotebookLayoutLocked() const
{
    return this->lockNotebookLayout_;
}

void Notebook::setLockNotebookLayout(bool value)
{
    this->lockNotebookLayout_ = value;
    this->lockNotebookLayoutAction_->setChecked(value);
    getSettings()->lockNotebookLayout.setValue(value);
}

void Notebook::addNotebookActionsToMenu(QMenu *menu)
{
    menu->addAction(
        "Toggle visibility of tabs",
        [this]() {
            this->setShowTabs(!this->getShowTabs());
        },
        QKeySequence("Ctrl+U"));

    menu->addAction(this->lockNotebookLayoutAction_);
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

size_t Notebook::visibleButtonCount() const
{
    size_t i = 0;
    for (auto *btn : this->customButtons_)
    {
        if (btn->isVisible())
        {
            ++i;
        }
    }
    return i;
}

SplitNotebook::SplitNotebook(Window *parent)
    : Notebook(parent)
{
    this->connect(this->getAddButton(), &NotebookButton::leftClicked, [this]() {
        QTimer::singleShot(80, this, [this] {
            this->addPage(true);
        });
    });

    // add custom buttons if they are not in the parent window frame
    if (!parent->hasCustomWindowFrame())
    {
        this->addCustomButtons();
    }

    this->signalHolder_.managedConnect(
        getApp()->windows->selectSplit, [this](Split *split) {
            for (auto &&item : this->items())
            {
                if (auto sc = dynamic_cast<SplitContainer *>(item.page))
                {
                    auto &&splits = sc->getSplits();
                    if (std::find(splits.begin(), splits.end(), split) !=
                        splits.end())
                    {
                        this->select(item.page);
                        split->setFocus();
                        break;
                    }
                }
            }
        });

    this->signalHolder_.managedConnect(getApp()->windows->selectSplitContainer,
                                       [this](SplitContainer *sc) {
                                           this->select(sc);
                                       });
}

void SplitNotebook::showEvent(QShowEvent *)
{
    if (auto page = this->getSelectedPage())
    {
        if (auto split = page->findChild<Split *>())
        {
            split->setFocus(Qt::FocusReason::OtherFocusReason);
        }
    }
}

void SplitNotebook::addCustomButtons()
{
    // settings
    auto settingsBtn = this->addCustomButton();

    settingsBtn->setVisible(!getSettings()->hidePreferencesButton.getValue());

    getSettings()->hidePreferencesButton.connect(
        [settingsBtn](bool hide, auto) {
            settingsBtn->setVisible(!hide);
        },
        this->signalHolder_);

    settingsBtn->setIcon(NotebookButton::Settings);

    QObject::connect(settingsBtn, &NotebookButton::leftClicked, [this] {
        getApp()->windows->showSettingsDialog(this);
    });

    // account
    auto userBtn = this->addCustomButton();
    userBtn->setVisible(!getSettings()->hideUserButton.getValue());
    getSettings()->hideUserButton.connect(
        [userBtn](bool hide, auto) {
            userBtn->setVisible(!hide);
        },
        this->signalHolder_);

    userBtn->setIcon(NotebookButton::User);
    QObject::connect(userBtn, &NotebookButton::leftClicked, [this, userBtn] {
        getApp()->windows->showAccountSelectPopup(
            this->mapToGlobal(userBtn->rect().bottomRight()));
    });

    // updates
    auto updateBtn = this->addCustomButton();

    initUpdateButton(*updateBtn, this->signalHolder_);
}

SplitContainer *SplitNotebook::addPage(bool select)
{
    auto container = new SplitContainer(this);
    auto tab = Notebook::addPage(container, QString(), select);
    container->setTab(tab);
    tab->setParent(this);
    tab->setVisible(this->getShowTabs());
    return container;
}

SplitContainer *SplitNotebook::getOrAddSelectedPage()
{
    auto *selectedPage = this->getSelectedPage();

    return selectedPage != nullptr ? (SplitContainer *)selectedPage
                                   : this->addPage();
}

void SplitNotebook::select(QWidget *page, bool focusPage)
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
    this->Notebook::select(page, focusPage);
}

}  // namespace chatterino
