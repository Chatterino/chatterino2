#pragma once

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
};

}  // namespace chatterino
