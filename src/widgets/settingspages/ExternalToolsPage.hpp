// SPDX-FileCopyrightText: 2018 Contributors to Chatterino <https://chatterino.com>
//
// SPDX-License-Identifier: MIT

#pragma once

#include "widgets/settingspages/GeneralPageView.hpp"
#include "widgets/settingspages/SettingsPage.hpp"

namespace chatterino {

class ExternalToolsPage : public SettingsPage
{
public:
    ExternalToolsPage();

    bool filterElements(const QString &query) override;

private:
    void initLayout(GeneralPageView &layout);

    GeneralPageView *view{};
};

}  // namespace chatterino
