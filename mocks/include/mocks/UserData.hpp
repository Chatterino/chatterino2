#pragma once

#include "controllers/userdata/UserDataController.hpp"

#include <unordered_map>

namespace chatterino::mock {

class UserDataController : public IUserDataController
{
public:
    UserDataController() = default;

    // Get extra data about a user.
    // If the user does not have any extra data, return none.
    std::optional<UserData> getUser(const QString &userID) const override
    {
        const auto it = this->userMap.find(userID);
        if (it != this->userMap.end())
        {
            return it->second;
        }

        return std::nullopt;
    }

    // Update or insert extra data for the user's color override.
    void setUserColor(const QString &userID,
                      const QString &colorString) override
    {
        if (userID.isEmpty())
        {
            return;
        }

        this->userMap[userID].color = QColor(colorString);
        this->userDataUpdated_.invoke();
    }

    void setUserNotes(const QString &userID, const QString &notes) override
    {
        if (userID.isEmpty())
        {
            return;
        }

        this->userMap[userID].notes = notes;
        this->userDataUpdated_.invoke();
    }

    void setUserAlias(const QString &userID, const QString &alias) override
    {
        if (userID.isEmpty())
        {
            return;
        }

        auto user = this->getUser(userID).value_or(UserData{});
        user.alias = alias.trimmed();

        if (user.isEmpty())
        {
            this->userMap.erase(userID);
        }
        else
        {
            this->userMap[userID] = user;
        }

        this->userDataUpdated_.invoke();
    }

    pajlada::Signals::NoArgSignal &userDataUpdated() override
    {
        return this->userDataUpdated_;
    }

private:
    std::unordered_map<QString, UserData> userMap;
    pajlada::Signals::NoArgSignal userDataUpdated_;
};

}  // namespace chatterino::mock
