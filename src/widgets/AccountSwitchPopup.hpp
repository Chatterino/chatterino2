#pragma once

#include "widgets/BaseWindow.hpp"

#include <QWidget>

namespace chatterino {

class AccountSwitchWidget;

class AccountSwitchPopup : public BaseWindow
{
    Q_OBJECT

public:
    AccountSwitchPopup(QWidget *parent = nullptr);

    void refresh();

protected:
    void focusOutEvent(QFocusEvent *event) final;
    void paintEvent(QPaintEvent *event) override;

    void themeChangedEvent() override;

private:
    struct {
        AccountSwitchWidget *accountSwitchWidget = nullptr;
    } ui_;
};

}  // namespace chatterino
