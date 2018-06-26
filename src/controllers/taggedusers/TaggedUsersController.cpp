#include "taggeduserscontroller.hpp"

#include "controllers/taggedusers/taggedusersmodel.hpp"

namespace chatterino {
namespace controllers {
namespace taggedusers {

TaggedUsersController::TaggedUsersController()
{
}

TaggedUsersModel *TaggedUsersController::createModel(QObject *parent)
{
    TaggedUsersModel *model = new TaggedUsersModel(parent);
    model->init(&this->users);

    return model;
}

}  // namespace taggedusers
}  // namespace controllers
}  // namespace chatterino
