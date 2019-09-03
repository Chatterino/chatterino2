#pragma once

#include "widgets/AccountSwitchWidget.hpp"

#include <QWidget>

namespace chatterino {

class AccountSwitchPopup : public QWidget
{
    Q_OBJECT

public:
    AccountSwitchPopup(QWidget *parent = nullptr);

    void refresh();

protected:
    void focusOutEvent(QFocusEvent *event) final;
    void paintEvent(QPaintEvent *event) override;

private:
    struct {
        AccountSwitchWidget *accountSwitchWidget = nullptr;
    } ui_;
};

}  // namespace chatterino
