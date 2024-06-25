#include "controllers/userdata/UserDataController.hpp"

#include "singletons/Paths.hpp"
#include "util/CombinePath.hpp"
#include "util/Helpers.hpp"

namespace {

using namespace chatterino;

std::shared_ptr<pajlada::Settings::SettingManager> initSettingsInstance(
    const Paths &paths)
{
    auto sm = std::make_shared<pajlada::Settings::SettingManager>();

    auto path = combinePath(paths.settingsDirectory, "user-data.json");

    sm->setPath(path.toUtf8().toStdString());

    sm->setBackupEnabled(true);
    sm->setBackupSlots(9);
    sm->saveMethod =
        pajlada::Settings::SettingManager::SaveMethod::SaveAllTheTime;

    return sm;
}

}  // namespace

namespace chatterino {

UserDataController::UserDataController(const Paths &paths)
    : sm(initSettingsInstance(paths))
    , setting("/users", this->sm)
{
    this->sm->load();
    this->users = this->setting.getValue();
}

std::optional<UserData> UserDataController::getUser(const QString &userID) const
{
    std::shared_lock lock(this->usersMutex);
    auto it = this->users.find(userID);

    if (it == this->users.end())
    {
        return std::nullopt;
    }

    return it->second;
}

std::unordered_map<QString, UserData> UserDataController::getUsers() const
{
    std::shared_lock lock(this->usersMutex);
    return this->users;
}

void UserDataController::setUserColor(const QString &userID,
                                      const QString &colorString)
{
    auto c = this->getUsers();
    auto it = c.find(userID);
    std::optional<QColor> finalColor =
        makeConditionedOptional(!colorString.isEmpty(), QColor(colorString));
    if (it == c.end())
    {
        if (!finalColor)
        {
            // Early out - user is not configured and will not get a new color
            return;
        }

        UserData user;
        user.color = finalColor;
        c.insert({userID, user});
    }
    else
    {
        it->second.color = finalColor;
    }

    this->update(std::move(c));
}

void UserDataController::update(
    std::unordered_map<QString, UserData> &&newUsers)
{
    std::unique_lock lock(this->usersMutex);
    this->users = std::move(newUsers);
    this->setting.setValue(this->users);
}

}  // namespace chatterino
