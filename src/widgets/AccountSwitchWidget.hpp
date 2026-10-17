// SPDX-FileCopyrightText: 2017 Contributors to Chatterino <https://chatterino.com>
//
// SPDX-License-Identifier: MIT

#pragma once

#include "pajlada/signals/signalholder.hpp"

#include <QListWidget>

namespace chatterino {

class AccountSwitchWidget : public QListWidget
{
    Q_OBJECT

public:
    explicit AccountSwitchWidget(QWidget *parent = nullptr);

    void refresh();

private:
    /// Rebuilds the list from the current accounts, marking expired ones
    void refreshList();
    void addAccountItem(const QString &userName, bool expired);
    void refreshSelection();

    pajlada::Signals::SignalHolder managedConnections_;
};

}  // namespace chatterino
