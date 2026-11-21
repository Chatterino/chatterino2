// SPDX-FileCopyrightText: 2018 Contributors to Chatterino <https://chatterino.com>
//
// SPDX-License-Identifier: MIT

#pragma once

#include "widgets/settingspages/SettingsPage.hpp"

#include <QStringListModel>

class QVBoxLayout;

namespace chatterino {

class IgnoresPage : public SettingsPage
{
public:
    IgnoresPage();

    void onShow() final;

private:
    QStringListModel userListModel_;
};

}  // namespace chatterino
