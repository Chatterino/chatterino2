// SPDX-FileCopyrightText: 2018 Contributors to Chatterino <https://chatterino.com>
//
// SPDX-License-Identifier: MIT

#pragma once

#include "widgets/settingspages/SettingsPage.hpp"

namespace chatterino {

class HotkeyModel;
class EditableModelView;

class KeyboardSettingsPage : public SettingsPage
{
public:
    KeyboardSettingsPage();
    bool filterElements(const QString &query) override;

private:
    EditableModelView *view_;
};

}  // namespace chatterino
