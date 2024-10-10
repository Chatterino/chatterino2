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

    // Update or insert extra data for the user's color override
    void setUserColor(const QString &userID,
                      const QString &colorString) override
    {
        auto it = this->userMap.find(userID);
        if (it != this->userMap.end())
        {
            it->second.color = QColor(colorString);
        }
        else
        {
            this->userMap.emplace(userID, UserData{
                                              .color = QColor(colorString),
                                          });
        }
    }

private:
    std::unordered_map<QString, UserData> userMap;
};

}  // namespace chatterino::mock
