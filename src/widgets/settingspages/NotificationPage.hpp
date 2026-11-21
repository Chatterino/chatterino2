// SPDX-FileCopyrightText: 2018 Contributors to Chatterino <https://chatterino.com>
//
// SPDX-License-Identifier: MIT

#pragma once

#include "widgets/settingspages/SettingsPage.hpp"

class QComboBox;

namespace chatterino {

class NotificationPage : public SettingsPage
{
public:
    NotificationPage();

private:
    QComboBox *createToastReactionComboBox();
};

}  // namespace chatterino
