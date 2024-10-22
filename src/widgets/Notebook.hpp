#pragma once

#include "widgets/BaseWidget.hpp"

#include <pajlada/signals/signal.hpp>
#include <pajlada/signals/signalholder.hpp>
#include <QList>
#include <QMenu>
#include <QMessageBox>
#include <QWidget>

#include <functional>

namespace chatterino {

class Window;
class UpdateDialog;
class NotebookButton;
class NotebookTab;
class SplitContainer;
class Split;

enum NotebookTabLocation { Top = 0, Left = 1, Right = 2, Bottom = 3 };

// Controls the visibility of tabs in this notebook
enum NotebookTabVisibility : int {
    // Show all tabs
    AllTabs = 0,

    // Only show tabs containing splits that are live
    LiveOnly = 1,
};

using TabVisibilityFilter = std::function<bool(const NotebookTab *)>;

class Notebook : public BaseWidget
{
    Q_OBJECT

public:
    explicit Notebook(QWidget *parent);
    ~Notebook() override = default;

    NotebookTab *addPage(QWidget *page, QString title = QString(),
                         bool select = false);

    /**
     * @brief Adds a page to the Notebook at a given position.
     *
     * @param position if set to -1, adds the page to the end
     **/
    NotebookTab *addPageAt(QWidget *page, int position,
                           QString title = QString(), bool select = false);
    void removePage(QWidget *page);
    void duplicatePage(QWidget *page);
    void removeCurrentPage();

    /**
     * @brief Returns index of page in Notebook, or -1 if not found.
     **/
    int indexOf(QWidget *page) const;

    /**
     * @brief Returns the visible index of page in Notebook, or -1 if not found.
     * Given page should be visible according to the set TabVisibilityFilter.
     **/
    int visibleIndexOf(QWidget *page) const;

    /**
     * @brief Returns the number of visible tabs in Notebook. 
     **/
    int getVisibleTabCount() const;

    /**
     * @brief Selects the Notebook tab containing the given page.
     **/
    virtual void select(QWidget *page, bool focusPage = true);

    /**
     * @brief Selects the Notebook tab at the given index. Ignores whether tabs
     * are visible or not. 
     **/
    void selectIndex(int index, bool focusPage = true);

    /**
     * @brief Selects the index'th visible tab in the Notebook.
     * 
     * For example, selecting the 0th visible tab selects the first tab in this 
     * Notebook that is visible according to the TabVisibilityFilter. If no filter
     * is set, equivalent to Notebook::selectIndex.
     **/
    void selectVisibleIndex(int index, bool focusPage = true);

    /**
     * @brief Selects the next visible tab. Wraps to the start if required. 
     **/
    void selectNextTab(bool focusPage = true);

    /**
     * @brief Selects the previous visible tab. Wraps to the end if required. 
     **/
    void selectPreviousTab(bool focusPage = true);

    /**
     * @brief Selects the last visible tab. 
     **/
    void selectLastTab(bool focusPage = true);

    int getPageCount() const;
    QWidget *getPageAt(int index) const;
    int getSelectedIndex() const;
    QWidget *getSelectedPage() const;

    QWidget *tabAt(QPoint point, int &index, int maxWidth = 2000000000);
    void rearrangePage(QWidget *page, int index);

    bool getAllowUserTabManagement() const;
    void setAllowUserTabManagement(bool value);

    bool getShowAddButton() const;
    void setShowAddButton(bool value);

    void setTabLocation(NotebookTabLocation location);

    bool isNotebookLayoutLocked() const;
    void setLockNotebookLayout(bool value);

    virtual void addNotebookActionsToMenu(QMenu *menu);

    // Update layout and tab visibility
    void refresh();

protected:
    bool getShowTabs() const;
    void setShowTabs(bool value);

    void scaleChangedEvent(float scale_) override;
    void resizeEvent(QResizeEvent *) override;
    void mousePressEvent(QMouseEvent *event) override;
    void paintEvent(QPaintEvent *) override;

    NotebookButton *getAddButton();
    NotebookButton *addCustomButton();

    struct Item {
        NotebookTab *tab{};
        QWidget *page{};
        QWidget *selectedWidget{};
    };

    const QList<Item> items()
    {
        return items_;
    }

    /**
     * @brief Apply the given tab visibility filter
     *
     * An empty function can be provided to denote that no filter will be applied
     *
     * Tabs will be redrawn after this function is called.
     **/
    void setTabVisibilityFilter(TabVisibilityFilter filter);

    /**
     * @brief shouldShowTab has the final say whether a tab should be visible right now.
     **/
    bool shouldShowTab(const NotebookTab *tab) const;

private:
    void performLayout(bool animate = false);

    /**
     * @brief Show a popup informing the user of some big tab visibility changes
     **/
    void showTabVisibilityInfoPopup();

    /**
     * @brief Updates the visibility state of all tabs
     **/
    void updateTabVisibility();
    void resizeAddButton();

    bool containsPage(QWidget *page);
    Item *findItem(QWidget *page);

    static bool containsChild(const QObject *obj, const QObject *child);
    NotebookTab *getTabFromPage(QWidget *page);

    // Returns the number of buttons in `customButtons_` that are visible
    size_t visibleButtonCount() const;

    QList<Item> items_;
    QMenu *menu_ = nullptr;
    QWidget *selectedPage_ = nullptr;

    NotebookButton *addButton_;
    std::vector<NotebookButton *> customButtons_;

    bool allowUserTabManagement_ = false;
    bool showTabs_ = true;
    bool showAddButton_ = false;
    int lineOffset_ = 20;
    bool lockNotebookLayout_ = false;

    bool refreshPaused_ = false;
    bool refreshRequested_ = false;

    NotebookTabLocation tabLocation_ = NotebookTabLocation::Top;

    QAction *lockNotebookLayoutAction_;
    QAction *toggleTopMostAction_;

    // This filter, if set, is used to figure out the visibility of
    // the tabs in this notebook.
    TabVisibilityFilter tabVisibilityFilter_;
};

class SplitNotebook : public Notebook
{
public:
    SplitNotebook(Window *parent);

    SplitContainer *addPage(bool select = false);
    SplitContainer *getOrAddSelectedPage();
    /// Returns `nullptr` when no page is selected.
    SplitContainer *getSelectedPage();
    void select(QWidget *page, bool focusPage = true) override;
    void themeChangedEvent() override;

    void addNotebookActionsToMenu(QMenu *menu) override;

    void forEachSplit(const std::function<void(Split *)> &cb);

    /**
     * Toggles between the "Show all tabs" and "Hide all tabs" tab visibility states
     */
    void toggleTabVisibility();

    QAction *showAllTabsAction;
    QAction *onlyShowLiveTabsAction;
    QAction *hideAllTabsAction;

protected:
    void showEvent(QShowEvent *event) override;

private:
    void addCustomButtons();

    pajlada::Signals::SignalHolder signalHolder_;

    // Main window on Windows has basically a duplicate of this in Window
    NotebookButton *streamerModeIcon_{};
    void updateStreamerModeIcon();
};

}  // namespace chatterino
