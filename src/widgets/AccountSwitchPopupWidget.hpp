#pragma once

#include "widgets/AccountSwitchWidget.hpp"

#include <QWidget>

namespace chatterino {
namespace widgets {

class AccountSwitchPopupWidget : public QWidget
{
    Q_OBJECT

public:
    AccountSwitchPopupWidget(QWidget *parent = nullptr);

    void refresh();

protected:
    void focusOutEvent(QFocusEvent *event) final;
    void paintEvent(QPaintEvent *event) override;

private:
    struct {
        AccountSwitchWidget *accountSwitchWidget = nullptr;
    } ui;
};

}  // namespace widgets
}  // namespace chatterino
