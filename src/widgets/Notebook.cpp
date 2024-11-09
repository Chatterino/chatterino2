#include "widgets/Notebook.hpp"

#include "Application.hpp"
#include "common/Args.hpp"
#include "common/QLogging.hpp"
#include "controllers/hotkeys/HotkeyCategory.hpp"
#include "controllers/hotkeys/HotkeyController.hpp"
#include "singletons/Resources.hpp"
#include "singletons/Settings.hpp"
#include "singletons/StreamerMode.hpp"
#include "singletons/Theme.hpp"
#include "singletons/WindowManager.hpp"
#include "util/InitUpdateButton.hpp"
#include "widgets/dialogs/SettingsDialog.hpp"
#include "widgets/helper/ChannelView.hpp"
#include "widgets/helper/NotebookButton.hpp"
#include "widgets/helper/NotebookTab.hpp"
#include "widgets/splits/Split.hpp"
#include "widgets/splits/SplitContainer.hpp"
#include "widgets/Window.hpp"

#include <boost/foreach.hpp>
#include <QActionGroup>
#include <QDebug>
#include <QFile>
#include <QFormLayout>
#include <QLayout>
#include <QList>
#include <QStandardPaths>
#include <QUuid>
#include <QWidget>

#include <utility>

namespace chatterino {

Notebook::Notebook(QWidget *parent)
    : BaseWidget(parent)
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

    this->toggleTopMostAction_ = new QAction("Top most window", this);
    this->toggleTopMostAction_->setCheckable(true);
    auto *window = dynamic_cast<BaseWindow *>(this->window());
    if (window)
    {
        auto updateTopMost = [this, window] {
            this->toggleTopMostAction_->setChecked(window->isTopMost());
        };
        updateTopMost();
        QObject::connect(this->toggleTopMostAction_, &QAction::triggered,
                         window, [window] {
                             window->setTopMost(!window->isTopMost());
                         });
        QObject::connect(window, &BaseWindow::topMostChanged, this,
                         updateTopMost);
    }
    else
    {
        qCWarning(chatterinoApp)
            << "Notebook must be created within a BaseWindow";
    }

    // Manually resize the add button so the initial paint uses the correct
    // width when computing the maximum width occupied per column in vertical
    // tab rendering.
    this->resizeAddButton();
}

NotebookTab *Notebook::addPage(QWidget *page, QString title, bool select)
{
    return this->addPageAt(page, -1, std::move(title), select);
}

NotebookTab *Notebook::addPageAt(QWidget *page, int position, QString title,
                                 bool select)
{
    // Queue up save because: Tab added
    getApp()->getWindows()->queueSave();

    auto *tab = new NotebookTab(this);
    tab->page = page;

    tab->setCustomTitle(title);
    tab->setTabLocation(this->tabLocation_);

    Item item;
    item.page = page;
    item.tab = tab;

    if (position == -1)
    {
        this->items_.push_back(item);
    }
    else
    {
        this->items_.insert(position, item);
    }

    page->hide();
    page->setParent(this);

    if (select || this->items_.count() == 1)
    {
        this->select(page);
    }

    this->performLayout();
    tab->setVisible(this->shouldShowTab(tab));
    return tab;
}

void Notebook::removePage(QWidget *page)
{
    // Queue up save because: Tab removed
    getApp()->getWindows()->queueSave();

    int removingIndex = this->indexOf(page);
    assert(removingIndex != -1);

    if (this->selectedPage_ == page)
    {
        // The page that we are removing is currently selected. We need to determine
        // the best tab to select before we remove this one. We follow a strategy used
        // by many web browsers: select the next tab. If there is no next tab, select
        // the previous tab.
        int countVisible = this->getVisibleTabCount();
        int visibleIndex = this->visibleIndexOf(page);
        assert(visibleIndex != -1);  // A selected page should always be visible

        if (this->items_.count() == 1)
        {
            // Deleting only tab, select nothing
            this->select(nullptr);
        }
        else if (countVisible == 1)
        {
            // Closing the only visible tab, try to select any tab (even if not visible)
            int nextIndex = (removingIndex + 1) % this->items_.count();
            this->select(this->items_[nextIndex].page);
        }
        else if (visibleIndex == countVisible - 1)
        {
            // Closing last visible tab, select the previous visible tab
            this->selectPreviousTab();
        }
        else
        {
            // Otherwise, select the next visible tab
            this->selectNextTab();
        }
    }

    // Remove page and delete resources
    this->items_[removingIndex].page->deleteLater();
    this->items_[removingIndex].tab->deleteLater();
    this->items_.removeAt(removingIndex);

    this->performLayout(true);
}

void Notebook::duplicatePage(QWidget *page)
{
    auto *item = this->findItem(page);
    assert(item != nullptr);
    if (item == nullptr)
    {
        return;
    }

    auto *container = dynamic_cast<SplitContainer *>(item->page);
    if (!container)
    {
        return;
    }

    auto *newContainer = new SplitContainer(this);
    if (!container->getSplits().empty())
    {
        auto descriptor = container->buildDescriptor();
        newContainer->applyFromDescriptor(descriptor);
    }

    const auto tabPosition = this->indexOf(page);
    auto newTabPosition = -1;
    if (tabPosition != -1)
    {
        newTabPosition = tabPosition + 1;
    }

    QString newTabTitle = "";
    if (item->tab->hasCustomTitle())
    {
        newTabTitle = item->tab->getCustomTitle();
    }

    auto *tab =
        this->addPageAt(newContainer, newTabPosition, newTabTitle, false);
    tab->copyHighlightStateAndSourcesFrom(item->tab);

    newContainer->setTab(tab);
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

int Notebook::visibleIndexOf(QWidget *page) const
{
    if (!this->tabVisibilityFilter_)
    {
        return this->indexOf(page);
    }

    int i = 0;
    for (const auto &item : this->items_)
    {
        if (item.page == page)
        {
            assert(this->tabVisibilityFilter_(item.tab));
            return i;
        }
        if (this->tabVisibilityFilter_(item.tab))
        {
            ++i;
        }
    }

    return -1;
}

int Notebook::getVisibleTabCount() const
{
    if (!this->tabVisibilityFilter_)
    {
        return this->items_.count();
    }

    int i = 0;
    for (const auto &item : this->items_)
    {
        if (this->tabVisibilityFilter_(item.tab))
        {
            ++i;
        }
    }
    return i;
}

void Notebook::select(QWidget *page, bool focusPage)
{
    if (page == this->selectedPage_)
    {
        // Nothing has changed
        return;
    }

    if (page)
    {
        // A new page has been selected, mark it as selected & focus one of its splits
        auto *item = this->findItem(page);
        if (!item)
        {
            return;
        }

        page->show();

        item->tab->setSelected(true);
        item->tab->raise();

        if (focusPage)
        {
            if (item->selectedWidget == nullptr)
            {
                item->page->setFocus();
            }
            else
            {
                if (containsChild(page, item->selectedWidget))
                {
                    item->selectedWidget->setFocus(Qt::MouseFocusReason);
                }
                else
                {
                    qCDebug(chatterinoWidget) << "Notebook: selected child of "
                                                 "page doesn't exist anymore";
                }
            }
        }
    }

    if (this->selectedPage_)
    {
        // Hide the previously selected page
        this->selectedPage_->hide();

        auto *item = this->findItem(selectedPage_);
        if (!item)
        {
            return;
        }
        item->tab->setSelected(false);
        item->selectedWidget = this->selectedPage_->focusWidget();
    }

    this->selectedPage_ = page;

    this->performLayout();
    this->updateTabVisibility();
}

bool Notebook::containsPage(QWidget *page)
{
    return std::any_of(this->items_.begin(), this->items_.end(),
                       [page](const auto &item) {
                           return item.page == page;
                       });
}

Notebook::Item *Notebook::findItem(QWidget *page)
{
    auto it = std::find_if(this->items_.begin(), this->items_.end(),
                           [page](const auto &item) {
                               return page == item.page;
                           });
    if (it != this->items_.end())
    {
        return &(*it);
    }
    return nullptr;
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

void Notebook::selectVisibleIndex(int index, bool focusPage)
{
    if (!this->tabVisibilityFilter_)
    {
        this->selectIndex(index, focusPage);
        return;
    }

    int i = 0;
    for (auto &item : this->items_)
    {
        if (this->tabVisibilityFilter_(item.tab))
        {
            if (i == index)
            {
                // found the index'th visible page
                this->select(item.page, focusPage);
                return;
            }
            ++i;
        }
    }
}

void Notebook::selectNextTab(bool focusPage)
{
    const int size = this->items_.size();

    if (!this->tabVisibilityFilter_)
    {
        if (size <= 1)
        {
            return;
        }

        auto index = (this->indexOf(this->selectedPage_) + 1) % size;
        this->select(this->items_[index].page, focusPage);
        return;
    }

    // find next tab that is permitted by filter
    const int startIndex = this->indexOf(this->selectedPage_);

    auto index = (startIndex + 1) % size;
    while (index != startIndex)
    {
        if (this->tabVisibilityFilter_(this->items_[index].tab))
        {
            this->select(this->items_[index].page, focusPage);
            return;
        }
        index = (index + 1) % size;
    }
}

void Notebook::selectPreviousTab(bool focusPage)
{
    const int size = this->items_.size();

    if (!this->tabVisibilityFilter_)
    {
        if (size <= 1)
        {
            return;
        }

        int index = this->indexOf(this->selectedPage_) - 1;
        if (index < 0)
        {
            index += size;
        }

        this->select(this->items_[index].page, focusPage);
        return;
    }

    // find next previous tab that is permitted by filter
    const int startIndex = this->indexOf(this->selectedPage_);

    auto index = startIndex == 0 ? size - 1 : startIndex - 1;
    while (index != startIndex)
    {
        if (this->tabVisibilityFilter_(this->items_[index].tab))
        {
            this->select(this->items_[index].page, focusPage);
            return;
        }

        index = index == 0 ? size - 1 : index - 1;
    }
}

void Notebook::selectLastTab(bool focusPage)
{
    if (!this->tabVisibilityFilter_)
    {
        const auto size = this->items_.size();
        if (size <= 1)
        {
            return;
        }

        this->select(this->items_[size - 1].page, focusPage);
        return;
    }

    // find first tab permitted by filter starting from the end
    for (auto it = this->items_.rbegin(); it != this->items_.rend(); ++it)
    {
        if (this->tabVisibilityFilter_(it->tab))
        {
            this->select(it->page, focusPage);
            return;
        }
    }
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
        if (!item.tab->isVisible())
        {
            i++;
            continue;
        }

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
    getApp()->getWindows()->queueSave();

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

    this->setShowAddButton(value);
    this->performLayout();

    this->updateTabVisibility();

    // show a popup upon hiding tabs
    if (!value && getSettings()->informOnTabVisibilityToggle.getValue())
    {
        this->showTabVisibilityInfoPopup();
    }
}

void Notebook::showTabVisibilityInfoPopup()
{
    auto unhideSeq = getApp()->getHotkeys()->getDisplaySequence(
        HotkeyCategory::Window, "setTabVisibility", {std::vector<QString>()});
    if (unhideSeq.isEmpty())
    {
        unhideSeq = getApp()->getHotkeys()->getDisplaySequence(
            HotkeyCategory::Window, "setTabVisibility", {{"toggle"}});
    }
    if (unhideSeq.isEmpty())
    {
        unhideSeq = getApp()->getHotkeys()->getDisplaySequence(
            HotkeyCategory::Window, "setTabVisibility", {{"on"}});
    }
    QString hotkeyInfo = "(currently unbound)";
    if (!unhideSeq.isEmpty())
    {
        hotkeyInfo =
            "(" + unhideSeq.toString(QKeySequence::SequenceFormat::NativeText) +
            ")";
    }
    QMessageBox msgBox(this->window());
    msgBox.window()->setWindowTitle("Chatterino - hidden tabs");
    msgBox.setText("You've just hidden your tabs.");
    msgBox.setInformativeText(
        "You can toggle tabs by using the keyboard shortcut " + hotkeyInfo +
        " or right-clicking the tab area and selecting \"Toggle "
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

void Notebook::refresh()
{
    if (this->refreshPaused_)
    {
        this->refreshRequested_ = true;
        return;
    }

    this->performLayout();
    this->updateTabVisibility();
}

void Notebook::updateTabVisibility()
{
    for (auto &item : this->items_)
    {
        item.tab->setVisible(this->shouldShowTab(item.tab));
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

void Notebook::scaleChangedEvent(float /*scale*/)
{
    this->resizeAddButton();
    this->refreshPaused_ = true;
    this->refreshRequested_ = false;
    for (auto &i : this->items_)
    {
        i.tab->updateSize();
    }
    this->refreshPaused_ = false;
    if (this->refreshRequested_)
    {
        this->refresh();
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
    const auto tabSpacer = std::max<int>(1, int(scale * 1));

    const auto buttonWidth = tabHeight;
    const auto buttonHeight = tabHeight - 1;

    std::vector<Item> filteredItems;
    filteredItems.reserve(this->items_.size());
    if (this->tabVisibilityFilter_)
    {
        std::copy_if(this->items_.begin(), this->items_.end(),
                     std::back_inserter(filteredItems),
                     [this](const auto &item) {
                         return this->tabVisibilityFilter_(item.tab);
                     });
    }
    else
    {
        filteredItems.assign(this->items_.begin(), this->items_.end());
    }

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
            auto *firstInBottomRow =
                filteredItems.empty() ? nullptr : &filteredItems.front();

            for (auto &item : filteredItems)
            {
                /// Break line if element doesn't fit.
                auto isFirst = &item == &filteredItems.front();
                auto isLast = &item == &filteredItems.back();

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
                x += item.tab->width() + tabSpacer;
            }

            /// Update which tabs are in the last row
            auto inLastRow = false;
            for (const auto &item : filteredItems)
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
        {
            y = tabHeight + lineThickness;  // account for divider line
        }

        int totalButtonWidths = x;
        const int top = y + tabSpacer;  // add margin

        y = top;
        x = left;

        // zneix: if we were to remove buttons when tabs are hidden
        // stuff below to "set page bounds" part should be in conditional statement
        int tabsPerColumn = (this->height() - top) / (tabHeight + tabSpacer);
        if (tabsPerColumn == 0)  // window hasn't properly rendered yet
        {
            return;
        }
        int count = filteredItems.size() + (this->showAddButton_ ? 1 : 0);
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
                    std::min(static_cast<size_t>((col + 1) * tabsPerColumn),
                             filteredItems.size());

                for (int i = tabStart; i < tabEnd; i++)
                {
                    largestWidth =
                        std::max(filteredItems.at(i).tab->normalTabWidth(),
                                 largestWidth);
                }

                if (isLastColumn && this->showAddButton_)
                {
                    largestWidth =
                        std::max(largestWidth, this->addButton_->width());
                }

                if (isLastColumn && largestWidth + x < totalButtonWidths)
                {
                    largestWidth = totalButtonWidths - x;
                }

                for (int i = tabStart; i < tabEnd; i++)
                {
                    auto item = filteredItems.at(i);

                    /// Layout tab
                    item.tab->growWidth(largestWidth);
                    item.tab->moveAnimated(QPoint(x, y), animated);
                    item.tab->setInLastRow(isLastColumn);
                    y += tabHeight + tabSpacer;
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
            auto *btn = *btnIt;
            if (!btn->isVisible())
            {
                continue;
            }

            x -= buttonWidth;
            btn->setFixedSize(buttonWidth, buttonHeight);
            btn->move(x, y);
        }

        if (this->visibleButtonCount() > 0)
        {
            y = tabHeight + lineThickness;  // account for divider line
        }

        int consumedButtonWidths = right - x;
        const int top = y + tabSpacer;  // add margin

        y = top;
        x = right;

        // zneix: if we were to remove buttons when tabs are hidden
        // stuff below to "set page bounds" part should be in conditional statement
        int tabsPerColumn = (this->height() - top) / (tabHeight + tabSpacer);
        if (tabsPerColumn == 0)  // window hasn't properly rendered yet
        {
            return;
        }
        int count = filteredItems.size() + (this->showAddButton_ ? 1 : 0);
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
                    std::min(static_cast<size_t>((col + 1) * tabsPerColumn),
                             filteredItems.size());

                for (int i = tabStart; i < tabEnd; i++)
                {
                    largestWidth =
                        std::max(filteredItems.at(i).tab->normalTabWidth(),
                                 largestWidth);
                }

                if (isLastColumn && this->showAddButton_)
                {
                    largestWidth =
                        std::max(largestWidth, this->addButton_->width());
                }

                int distanceFromRight = width() - x;

                if (isLastColumn &&
                    largestWidth + distanceFromRight < consumedButtonWidths)
                {
                    largestWidth = consumedButtonWidths - distanceFromRight;
                }

                x -= largestWidth + lineThickness;

                for (int i = tabStart; i < tabEnd; i++)
                {
                    auto item = filteredItems.at(i);

                    /// Layout tab
                    item.tab->growWidth(largestWidth);
                    item.tab->moveAnimated(QPoint(x, y), animated);
                    item.tab->setInLastRow(isLastColumn);
                    y += tabHeight + tabSpacer;
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
            y = bottom - tabHeight - tabSpacer;

            // layout tabs
            /// Notebook tabs need to know if they are in the last row.
            auto *firstInBottomRow =
                filteredItems.empty() ? nullptr : &filteredItems.front();

            for (auto &item : filteredItems)
            {
                /// Break line if element doesn't fit.
                auto isFirst = &item == &filteredItems.front();
                auto isLast = &item == &filteredItems.back();

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
                x += item.tab->width() + tabSpacer;
            }

            /// Update which tabs are in the last row
            auto inLastRow = false;
            for (const auto &item : filteredItems)
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
        int tabsStart = bottom - consumedBottomSpace - lineThickness;

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
            event->accept();

            if (!this->menu_)
            {
                this->menu_ = new QMenu(this);
                this->addNotebookActionsToMenu(this->menu_);
            }
            this->menu_->popup(event->globalPos() + QPoint(0, 8));
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

        // Update all tabs
        for (const auto &item : this->items_)
        {
            item.tab->setTabLocation(location);
        }

        this->performLayout();
    }
}

void Notebook::paintEvent(QPaintEvent *event)
{
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
    menu->addAction(this->lockNotebookLayoutAction_);

    menu->addAction(this->toggleTopMostAction_);
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

void Notebook::setTabVisibilityFilter(TabVisibilityFilter filter)
{
    if (filter)
    {
        // Wrap tab filter to always accept selected tabs. This prevents confusion
        // when jumping to hidden tabs with the quick switcher, for example.
        filter = [originalFilter = std::move(filter)](const NotebookTab *tab) {
            return tab->isSelected() || originalFilter(tab);
        };
    }

    this->tabVisibilityFilter_ = std::move(filter);
    this->performLayout();
    this->updateTabVisibility();
}

bool Notebook::shouldShowTab(const NotebookTab *tab) const
{
    if (!this->showTabs_)
    {
        return false;
    }

    if (this->tabVisibilityFilter_)
    {
        return this->tabVisibilityFilter_(tab);
    }

    return true;
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

    auto *tabVisibilityActionGroup = new QActionGroup(this);
    tabVisibilityActionGroup->setExclusionPolicy(
        QActionGroup::ExclusionPolicy::Exclusive);

    this->showAllTabsAction = new QAction("Show all tabs", this);
    this->showAllTabsAction->setCheckable(true);
    this->showAllTabsAction->setShortcut(
        getApp()->getHotkeys()->getDisplaySequence(
            HotkeyCategory::Window, "setTabVisibility", {{"on"}}));
    QObject::connect(this->showAllTabsAction, &QAction::triggered, this,
                     [this] {
                         this->setShowTabs(true);
                         getSettings()->tabVisibility.setValue(
                             NotebookTabVisibility::AllTabs);
                         this->showAllTabsAction->setChecked(true);
                     });
    tabVisibilityActionGroup->addAction(this->showAllTabsAction);

    this->onlyShowLiveTabsAction = new QAction("Only show live tabs", this);
    this->onlyShowLiveTabsAction->setCheckable(true);
    this->onlyShowLiveTabsAction->setShortcut(
        getApp()->getHotkeys()->getDisplaySequence(
            HotkeyCategory::Window, "setTabVisibility", {{"liveOnly"}}));
    QObject::connect(this->onlyShowLiveTabsAction, &QAction::triggered, this,
                     [this] {
                         this->setShowTabs(true);
                         getSettings()->tabVisibility.setValue(
                             NotebookTabVisibility::LiveOnly);
                         this->onlyShowLiveTabsAction->setChecked(true);
                     });
    tabVisibilityActionGroup->addAction(this->onlyShowLiveTabsAction);

    this->hideAllTabsAction = new QAction("Hide all tabs", this);
    this->hideAllTabsAction->setCheckable(true);
    this->hideAllTabsAction->setShortcut(
        getApp()->getHotkeys()->getDisplaySequence(
            HotkeyCategory::Window, "setTabVisibility", {{"off"}}));
    QObject::connect(this->hideAllTabsAction, &QAction::triggered, this,
                     [this] {
                         this->setShowTabs(false);
                         getSettings()->tabVisibility.setValue(
                             NotebookTabVisibility::AllTabs);
                         this->hideAllTabsAction->setChecked(true);
                     });
    tabVisibilityActionGroup->addAction(this->hideAllTabsAction);

    switch (getSettings()->tabVisibility.getEnum())
    {
        case NotebookTabVisibility::AllTabs: {
            this->showAllTabsAction->setChecked(true);
        }
        break;

        case NotebookTabVisibility::LiveOnly: {
            this->onlyShowLiveTabsAction->setChecked(true);
        }
        break;
    }

    getSettings()->tabVisibility.connect(
        [this](int val, auto) {
            auto visibility = NotebookTabVisibility(val);
            // Set the correct TabVisibilityFilter for the given visiblity setting.
            // Note that selected tabs are always shown regardless of what the tab
            // filter returns, so no need to include `tab->isSelected()` in the
            // predicate. See Notebook::setTabVisibilityFilter.
            switch (visibility)
            {
                case NotebookTabVisibility::LiveOnly:
                    this->setTabVisibilityFilter([](const NotebookTab *tab) {
                        return tab->isLive();
                    });
                    break;
                case NotebookTabVisibility::AllTabs:
                default:
                    this->setTabVisibilityFilter(nullptr);
                    break;
            }
        },
        this->signalHolder_, true);

    this->signalHolder_.managedConnect(
        getApp()->getWindows()->selectSplit, [this](Split *split) {
            for (auto &&item : this->items())
            {
                if (auto *sc = dynamic_cast<SplitContainer *>(item.page))
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

    this->signalHolder_.managedConnect(
        getApp()->getWindows()->selectSplitContainer,
        [this](SplitContainer *sc) {
            this->select(sc);
        });

    this->signalHolder_.managedConnect(
        getApp()->getWindows()->scrollToMessageSignal,
        [this](const MessagePtr &message) {
            for (auto &&item : this->items())
            {
                if (auto *sc = dynamic_cast<SplitContainer *>(item.page))
                {
                    for (auto *split : sc->getSplits())
                    {
                        auto type = split->getChannel()->getType();
                        if (type != Channel::Type::TwitchMentions &&
                            type != Channel::Type::TwitchAutomod)
                        {
                            if (split->getChannelView().scrollToMessage(
                                    message))
                            {
                                return;
                            }
                        }
                    }
                }
            }
        });
}

void SplitNotebook::addNotebookActionsToMenu(QMenu *menu)
{
    Notebook::addNotebookActionsToMenu(menu);

    auto *submenu = menu->addMenu("Tab visibility");
    submenu->addAction(this->showAllTabsAction);
    submenu->addAction(this->onlyShowLiveTabsAction);
    submenu->addAction(this->hideAllTabsAction);
}

void SplitNotebook::toggleTabVisibility()
{
    if (this->getShowTabs())
    {
        this->hideAllTabsAction->trigger();
    }
    else
    {
        this->showAllTabsAction->trigger();
    }
}

void SplitNotebook::showEvent(QShowEvent * /*event*/)
{
    if (auto *page = this->getSelectedPage())
    {
        auto *split = page->getSelectedSplit();
        if (!split)
        {
            split = page->findChild<Split *>();
        }

        if (split)
        {
            split->setFocus(Qt::OtherFocusReason);
        }
    }
}

void SplitNotebook::addCustomButtons()
{
    // settings
    auto *settingsBtn = this->addCustomButton();

    // This is to ensure you can't lock yourself out of the settings
    if (getApp()->getArgs().safeMode)
    {
        settingsBtn->setVisible(true);
    }
    else
    {
        settingsBtn->setVisible(
            !getSettings()->hidePreferencesButton.getValue());

        getSettings()->hidePreferencesButton.connect(
            [settingsBtn](bool hide, auto) {
                settingsBtn->setVisible(!hide);
            },
            this->signalHolder_);
    }

    settingsBtn->setIcon(NotebookButton::Settings);

    QObject::connect(settingsBtn, &NotebookButton::leftClicked, [this] {
        getApp()->getWindows()->showSettingsDialog(this);
    });

    // account
    auto *userBtn = this->addCustomButton();
    userBtn->setVisible(!getSettings()->hideUserButton.getValue());
    getSettings()->hideUserButton.connect(
        [userBtn](bool hide, auto) {
            userBtn->setVisible(!hide);
        },
        this->signalHolder_);

    userBtn->setIcon(NotebookButton::User);
    QObject::connect(userBtn, &NotebookButton::leftClicked, [this, userBtn] {
        getApp()->getWindows()->showAccountSelectPopup(
            this->mapToGlobal(userBtn->rect().bottomRight()));
    });

    // updates
    auto *updateBtn = this->addCustomButton();

    initUpdateButton(*updateBtn, this->signalHolder_);

    // streamer mode
    this->streamerModeIcon_ = this->addCustomButton();
    QObject::connect(this->streamerModeIcon_, &NotebookButton::leftClicked,
                     [this] {
                         getApp()->getWindows()->showSettingsDialog(
                             this, SettingsDialogPreference::StreamerMode);
                     });
    QObject::connect(getApp()->getStreamerMode(), &IStreamerMode::changed, this,
                     &SplitNotebook::updateStreamerModeIcon);
    this->updateStreamerModeIcon();
}

void SplitNotebook::updateStreamerModeIcon()
{
    if (this->streamerModeIcon_ == nullptr)
    {
        return;
    }
    // A duplicate of this code is in Window class
    // That copy handles the TitleBar icon in Window (main window on Windows)
    // This one is the one near splits (on linux and mac or non-main windows on Windows)
    if (getTheme()->isLightTheme())
    {
        this->streamerModeIcon_->setPixmap(
            getResources().buttons.streamerModeEnabledLight);
    }
    else
    {
        this->streamerModeIcon_->setPixmap(
            getResources().buttons.streamerModeEnabledDark);
    }
    this->streamerModeIcon_->setVisible(
        getApp()->getStreamerMode()->isEnabled());
}

void SplitNotebook::themeChangedEvent()
{
    this->updateStreamerModeIcon();
}

SplitContainer *SplitNotebook::addPage(bool select)
{
    auto *container = new SplitContainer(this);
    auto *tab = Notebook::addPage(container, QString(), select);
    container->setTab(tab);
    tab->setParent(this);
    return container;
}

SplitContainer *SplitNotebook::getOrAddSelectedPage()
{
    auto *selectedPage = this->getSelectedPage();

    if (selectedPage)
    {
        return dynamic_cast<SplitContainer *>(selectedPage);
    }

    return this->addPage();
}

SplitContainer *SplitNotebook::getSelectedPage()
{
    return dynamic_cast<SplitContainer *>(Notebook::getSelectedPage());
}

void SplitNotebook::select(QWidget *page, bool focusPage)
{
    // If there's a previously selected page, go through its splits and
    // update their "last read message" indicator
    if (auto *selectedPage = this->getSelectedPage())
    {
        if (auto *splitContainer = dynamic_cast<SplitContainer *>(selectedPage))
        {
            for (auto *split : splitContainer->getSplits())
            {
                split->updateLastReadMessage();
            }
        }
    }

    this->Notebook::select(page, focusPage);
}

void SplitNotebook::forEachSplit(const std::function<void(Split *)> &cb)
{
    for (const auto &item : this->items())
    {
        auto *page = dynamic_cast<SplitContainer *>(item.page);
        if (!page)
        {
            continue;
        }
        for (auto *split : page->getSplits())
        {
            cb(split);
        }
    }
}

}  // namespace chatterino
