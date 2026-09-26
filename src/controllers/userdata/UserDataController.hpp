// SPDX-FileCopyrightText: 2022 Contributors to Chatterino <https://chatterino.com>
//
// SPDX-License-Identifier: MIT

#pragma once

#include "controllers/userdata/UserData.hpp"
#include "util/QStringHash.hpp"
#include "util/RapidjsonHelpers.hpp"
#include "util/RapidJsonSerializeQString.hpp"
#include "util/serialize/Container.hpp"

#include <pajlada/settings.hpp>
#include <pajlada/signals/signal.hpp>
#include <QColor>
#include <QString>

#include <optional>
#include <shared_mutex>
#include <unordered_map>

namespace chatterino {

class Paths;

class IUserDataController
{
public:
    virtual ~IUserDataController() = default;

    virtual std::optional<UserData> getUser(const QString &userID) const = 0;
    virtual std::unordered_map<QString, UserData> getUsers() const = 0;

    virtual void setUserColor(const QString &userID,
                              const QString &colorString) = 0;
    virtual void setUserNotes(const QString &userID, const QString &notes) = 0;
    virtual void setUserNickname(const QString &userID, const QString &username,
                                 const QString &nickname) = 0;
    virtual void updateLastSeenUsername(const QString &userID,
                                        const QString &username) = 0;

    virtual pajlada::Signals::NoArgSignal &userDataUpdated() = 0;
};

class UserDataController : public IUserDataController
{
public:
    explicit UserDataController(const Paths &paths);

    // Get extra data about a user
    // If the user does not have any extra data, return none
    std::optional<UserData> getUser(const QString &userID) const override;
    std::unordered_map<QString, UserData> getUsers() const override;

    // Update or insert extra data for the user's color override
    void setUserColor(const QString &userID,
                      const QString &colorString) override;

    // Update or insert extra data for the notes about a user
    void setUserNotes(const QString &userID, const QString &notes) override;
    void setUserNickname(const QString &userID, const QString &username,
                         const QString &nickname) override;
    void updateLastSeenUsername(const QString &userID,
                                const QString &username) override;

    pajlada::Signals::NoArgSignal &userDataUpdated() override;

private:
    void update(std::unordered_map<QString, UserData> &&newUsers,
                std::unique_lock<std::shared_mutex> usersLock);

    // Stores a real-time list of users & their customizations
    std::unordered_map<QString, UserData> users;
    mutable std::shared_mutex usersMutex;

    std::shared_ptr<pajlada::Settings::SettingManager> sm;
    pajlada::Settings::Setting<std::unordered_map<QString, UserData>> setting;
    pajlada::Signals::NoArgSignal userDataUpdated_;
};

}  // namespace chatterino
