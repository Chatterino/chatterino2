#pragma once

#include "controllers/userdata/UserDataController.hpp"

namespace chatterino::mock {

class UserDataController : public IUserDataController
{
public:
    UserDataController() = default;

    // Get extra data about a user
    // If the user does not have any extra data, return none
    boost::optional<UserData> getUser(const QString &userID) const override
    {
        return boost::none;
    }

    // Update or insert extra data for the user's color override
    void setUserColor(const QString &userID,
                      const QString &colorString) override
    {
        // do nothing
    }
};

}  // namespace chatterino::mock
