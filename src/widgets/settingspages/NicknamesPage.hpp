// SPDX-FileCopyrightText: 2018 Contributors to Chatterino <https://chatterino.com>
//
// SPDX-License-Identifier: MIT

#pragma once

#include "controllers/nicknames/Nickname.hpp"
#include "controllers/userdata/UserData.hpp"
#include "widgets/settingspages/SettingsPage.hpp"

#include <QMap>

#include <optional>
#include <vector>

namespace chatterino {

class EditableModelView;

class NicknamesPage : public SettingsPage
{
public:
    NicknamesPage();
    bool filterElements(const QString &query) override;
    void onShow() override;
    void onSettingsDialogAccepted() override;
    void onSettingsDialogRejected() override;

private:
    void rememberAccountNickname(const QString &userID);
    void rememberLegacyNicknames();
    void restoreLegacyNicknames();

    EditableModelView *view_;
    QMap<QString, std::optional<UserData>> originalAccountNicknames_;
    std::vector<Nickname> originalLegacyNicknames_;
    bool legacyNicknamesChanged_{};
};

}  // namespace chatterino
