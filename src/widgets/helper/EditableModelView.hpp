// SPDX-FileCopyrightText: 2018 Contributors to Chatterino <https://chatterino.com>
//
// SPDX-License-Identifier: MIT

#pragma once

#include <pajlada/signals/signal.hpp>
#include <QKeySequence>
#include <QWidget>

#include <functional>
#include <span>

class QAbstractTableModel;
class QTableView;
class QHBoxLayout;
class QPushButton;

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
    void setRowRemovalPredicate(std::function<bool(int)> predicate);

    bool filterSearchResults(const QString &query,
                             std::span<const int> columnSelect);
    void filterSearchResultsHotkey(const QKeySequence &keySequenceQuery);

private:
    QTableView *tableView_{};
    QAbstractTableModel *model_{};
    QHBoxLayout *buttons_{};
    QPushButton *removeButton_{};
    std::function<bool(int)> rowRemovalPredicate_;

    void moveRow(int dir);
    void updateRemoveButton();
};

}  // namespace chatterino
