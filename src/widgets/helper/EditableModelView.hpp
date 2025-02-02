#pragma once

#include <pajlada/signals/signal.hpp>
#include <QKeySequence>
#include <QWidget>

#include <span>

class QAbstractTableModel;
class QTableView;
class QHBoxLayout;

namespace chatterino {

class EditableModelView : public QWidget
{
public:
    EditableModelView(QAbstractTableModel *model, bool movable = true);

    void setTitles(std::initializer_list<QString> titles);
    void setValidationRegexp(QRegularExpression regexp);

    QTableView *getTableView();
    QAbstractTableModel *getModel();

    pajlada::Signals::NoArgSignal addButtonPressed;

    void addCustomButton(QWidget *widget);
    void addRegexHelpLink();

    bool filterSearchResults(const QString &query,
                             std::span<const int> columnSelect);
    void filterSearchResultsHotkey(const QKeySequence &keySequenceQuery);

private:
    QTableView *tableView_{};
    QAbstractTableModel *model_{};
    QHBoxLayout *buttons_{};

    void moveRow(int dir);
};

}  // namespace chatterino
