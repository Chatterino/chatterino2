#pragma once

#include "widgets/listview/GenericItemDelegate.hpp"

#include <QListView>

namespace chatterino {

class GenericListModel;
class Theme;

class GenericListView : public QListView
{
    Q_OBJECT

public:
    GenericListView();

    void setModel(QAbstractItemModel *model) override;
    void setModel(GenericListModel *);
    void setInvokeActionOnTab(bool);
    bool eventFilter(QObject *watched, QEvent *event) override;

    GenericListModel *model_{};
    SwitcherItemDelegate itemDelegate_;

    void refreshTheme(const Theme &theme);

Q_SIGNALS:
    void closeRequested();

private:
    bool invokeActionOnTab_{};

    /**
     * @brief Gets the currently selected item (if any) and calls its action
     *
     * @return true if an action was called on an item, false if no item was selected and thus no action was called
     **/
    bool acceptCompletion();

    /**
     * @brief Select the next item in the list. Wraps around if the bottom of the list has been reached.
     **/
    void focusNextCompletion();

    /**
     * @brief Select the previous item in the list. Wraps around if the top of the list has been reached.
     **/
    void focusPreviousCompletion();

    /**
     * @brief Request for the GUI powering this list view to be closed. Shorthand for emit this->closeRequested()
     **/
    void requestClose();
};

}  // namespace chatterino
