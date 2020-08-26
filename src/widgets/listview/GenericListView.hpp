#pragma once

#include <QListView>
#include "widgets/listview/GenericItemDelegate.hpp"
#include "widgets/listview/GenericListItem.hpp"

namespace chatterino {

class GenericListModel;
class Theme;

class GenericListView : public QListView
{
    Q_OBJECT

public:
    GenericListView();

    virtual void setModel(QAbstractItemModel *model) override;
    void setModel(GenericListModel *);
    void setInvokeActionOnTab(bool);
    bool eventFilter(QObject *watched, QEvent *event) override;

    GenericListModel *model_{};
    SwitcherItemDelegate itemDelegate_;

    void refreshTheme(const Theme &theme);

signals:
    void closeRequested();

private:
    bool invokeActionOnTab_{};
};

}  // namespace chatterino
