#pragma once

#include "widgets/accountswitchwidget.hpp"

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
    virtual void focusOutEvent(QFocusEvent *event) override final;
    virtual void paintEvent(QPaintEvent *event) override;

private:
    struct {
        AccountSwitchWidget *accountSwitchWidget = nullptr;
    } ui;
};

}  // namespace widgets
}  // namespace chatterino
