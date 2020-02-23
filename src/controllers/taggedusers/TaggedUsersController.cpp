#include "TaggedUsersController.hpp"

#include "controllers/taggedusers/TaggedUsersModel.hpp"

namespace chatterino {

TaggedUsersController::TaggedUsersController()
    : users(std::less<TaggedUser>{})
{
}

}  // namespace chatterino
