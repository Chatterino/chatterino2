#pragma once

#include "controllers/userdata/UserData.hpp"
#include "util/QStringHash.hpp"
#include "util/RapidjsonHelpers.hpp"
#include "util/RapidJsonSerializeQString.hpp"
#include "util/serialize/Container.hpp"

#include <pajlada/settings.hpp>
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

    virtual void setUserColor(const QString &userID,
                              const QString &colorString) = 0;
};

class UserDataController : public IUserDataController
{
public:
    explicit UserDataController(const Paths &paths);

    // Get extra data about a user
    // If the user does not have any extra data, return none
    std::optional<UserData> getUser(const QString &userID) const override;

    // Update or insert extra data for the user's color override
    void setUserColor(const QString &userID,
                      const QString &colorString) override;

private:
    void update(std::unordered_map<QString, UserData> &&newUsers);

    std::unordered_map<QString, UserData> getUsers() const;

    // Stores a real-time list of users & their customizations
    std::unordered_map<QString, UserData> users;
    mutable std::shared_mutex usersMutex;

    std::shared_ptr<pajlada::Settings::SettingManager> sm;
    pajlada::Settings::Setting<std::unordered_map<QString, UserData>> setting;
};

}  // namespace chatterino
