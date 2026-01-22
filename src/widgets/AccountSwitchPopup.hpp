// SPDX-FileCopyrightText: 2017 Contributors to Chatterino <https://chatterino.com>
//
// SPDX-License-Identifier: MIT

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
    void paintEvent(QPaintEvent *event) override;

    void themeChangedEvent() override;

private:
    struct {
        AccountSwitchWidget *accountSwitchWidget = nullptr;
    } ui_;
};

}  // namespace chatterino
