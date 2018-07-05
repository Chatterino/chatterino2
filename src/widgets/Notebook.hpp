#pragma once

#include "pajlada/signals/signal.hpp"
#include "widgets/BaseWidget.hpp"
#include "widgets/dialogs/UpdatePromptDialog.hpp"
#include "widgets/helper/NotebookButton.hpp"
#include "widgets/helper/NotebookTab.hpp"
#include "widgets/splits/SplitContainer.hpp"

#include <QList>
#include <QMessageBox>
#include <QWidget>
#include <pajlada/signals/signalholder.hpp>

namespace chatterino {

class Window;

class Notebook : public BaseWidget
{
    Q_OBJECT

public:
    explicit Notebook(QWidget *parent);

    NotebookTab *addPage(QWidget *page, QString title = QString(), bool select = false);
    void removePage(QWidget *page);
    void removeCurrentPage();

    int indexOf(QWidget *page) const;
    void select(QWidget *page);
    void selectIndex(int index);
    void selectNextTab();
    void selectPreviousTab();

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

    void performLayout(bool animate = false);

protected:
    virtual void scaleChangedEvent(float scale) override;
    virtual void resizeEvent(QResizeEvent *) override;
    virtual void paintEvent(QPaintEvent *) override;

    NotebookButton *getAddButton();
    NotebookButton *addCustomButton();

private:
    struct Item {
        NotebookTab *tab;
        QWidget *page;
        QWidget *selectedWidget = nullptr;
    };

    QList<Item> items;
    QWidget *selectedPage = nullptr;

    bool containsPage(QWidget *page);
    Item &findItem(QWidget *page);

    static bool containsChild(const QObject *obj, const QObject *child);

    NotebookButton addButton;
    std::vector<NotebookButton *> customButtons;

    bool allowUserTabManagement = false;
    bool showAddButton = false;
    int lineY = 20;

    NotebookTab *getTabFromPage(QWidget *page);
};

class SplitNotebook : public Notebook, pajlada::Signals::SignalHolder
{
public:
    SplitNotebook(Window *parent);

    SplitContainer *addPage(bool select = false);
    SplitContainer *getOrAddSelectedPage();

private:
    void addCustomButtons();

    pajlada::Signals::SignalHolder signalHolder_;
    std::unique_ptr<UpdatePromptDialog> updateDialogHandle_;

    std::vector<pajlada::Signals::ScopedConnection> uniqueConnections;
};

}  // namespace chatterino
