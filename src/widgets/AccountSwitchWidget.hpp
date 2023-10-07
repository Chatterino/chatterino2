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
    void refreshSelection();

    pajlada::Signals::SignalHolder managedConnections_;
};

}  // namespace chatterino
