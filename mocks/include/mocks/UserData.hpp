#pragma once

#include "controllers/userdata/UserDataController.hpp"

#include <unordered_map>

namespace chatterino::mock {

class UserDataController : public IUserDataController
{
public:
    UserDataController() = default;

    // Get extra data about a user
    // If the user does not have any extra data, return none
    std::optional<UserData> getUser(const QString &userID) const override
    {
        auto it = this->userMap.find(userID);
        if (it != this->userMap.end())
        {
            return it->second;
        }
        return std::nullopt;
    }

    std::unordered_map<QString, UserData> getUsers() const override
    {
        return this->userMap;
    }

    // Update or insert extra data for the user's color override
    void setUserColor(const QString &userID,
                      const QString &colorString) override
    {
        this->userMap[userID].color = QColor(colorString);
        this->userDataUpdated_.invoke();
    }

    void setUserNotes(const QString &userID, const QString &notes) override
    {
        this->userMap[userID].notes = notes;
        this->userDataUpdated_.invoke();
    }

    void setUserNickname(const QString &userID, const QString &username,
                         const QString &nickname) override
    {
        const auto trimmedNickname = nickname.trimmed();
        const auto current = this->userMap.find(userID);
        if (current == this->userMap.end() && trimmedNickname.isEmpty())
        {
            return;
        }
        if (current != this->userMap.end())
        {
            const bool nicknameUnchanged =
                current->second.nickname == trimmedNickname;
            const bool usernameNeedsUpdate =
                !trimmedNickname.isEmpty() && !username.isEmpty() &&
                current->second.lastSeenUsername != username;
            if (nicknameUnchanged && !usernameNeedsUpdate)
            {
                return;
            }
        }

        auto &user = this->userMap[userID];
        user.nickname = trimmedNickname;
        if (trimmedNickname.isEmpty())
        {
            user.lastSeenUsername.clear();
        }
        else if (!username.isEmpty())
        {
            user.lastSeenUsername = username;
        }
        if (user.isEmpty())
        {
            this->userMap.erase(userID);
        }
        this->userDataUpdated_.invoke();
    }

    void updateLastSeenUsername(const QString &userID,
                                const QString &username) override
    {
        auto it = this->userMap.find(userID);
        if (it == this->userMap.end() || it->second.nickname.isEmpty() ||
            it->second.lastSeenUsername == username)
        {
            return;
        }
        it->second.lastSeenUsername = username;
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
