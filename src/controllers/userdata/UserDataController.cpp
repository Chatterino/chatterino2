#include "controllers/userdata/UserDataController.hpp"

#include "singletons/Paths.hpp"
#include "util/CombinePath.hpp"

namespace {

using namespace chatterino;

std::shared_ptr<pajlada::Settings::SettingManager> initSettingsInstance()
{
    auto sm = std::make_shared<pajlada::Settings::SettingManager>();

    auto *paths = getPaths();

    auto path = combinePath(paths->settingsDirectory, "user-data.json");

    sm->setPath(path.toUtf8().toStdString());

    sm->setBackupEnabled(true);
    sm->setBackupSlots(9);
    sm->saveMethod =
        pajlada::Settings::SettingManager::SaveMethod::SaveAllTheTime;

    return sm;
}

}  // namespace

namespace chatterino {

UserDataController::UserDataController()
    : sm(initSettingsInstance())
    , setting("/users", this->sm)
{
    this->sm->load();
    this->users = this->setting.getValue();
}

void UserDataController::save()
{
    this->sm->save();
}

boost::optional<UserData> UserDataController::getUser(
    const QString &userID) const
{
    auto it = this->users.find(userID);

    if (it == this->users.end())
    {
        return boost::none;
    }

    return it->second;
}

void UserDataController::setUserColor(const QString &userID,
                                      const QString &colorString)
{
    auto c = this->users;
    auto it = c.find(userID);
    boost::optional<QColor> finalColor =
        boost::make_optional(!colorString.isEmpty(), QColor(colorString));
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
    this->users = std::move(newUsers);
    this->setting.setValue(this->users);
}

}  // namespace chatterino
