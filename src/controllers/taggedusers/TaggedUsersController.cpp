#include "TaggedUsersController.hpp"

#include "controllers/taggedusers/TaggedUsersModel.hpp"

namespace chatterino {

TaggedUsersController::TaggedUsersController()
    : users(std::less<TaggedUser>{})
{
}

TaggedUsersModel *TaggedUsersController::createModel(QObject *parent)
{
    TaggedUsersModel *model = new TaggedUsersModel(parent);
    model->initialize(&this->users);

    return model;
}

}  // namespace chatterino
