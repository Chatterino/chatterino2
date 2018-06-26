#pragma once

#include "controllers/taggedusers/TaggedUser.hpp"
#include "util/SignalVector2.hpp"

namespace chatterino {
namespace controllers {
namespace taggedusers {

class TaggedUsersModel;

class TaggedUsersController
{
public:
    TaggedUsersController();

    util::SortedSignalVector<TaggedUser, std::less<TaggedUser>> users;

    TaggedUsersModel *createModel(QObject *parent = nullptr);
};

}  // namespace taggedusers
}  // namespace controllers
}  // namespace chatterino
