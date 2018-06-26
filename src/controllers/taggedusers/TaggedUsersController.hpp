#pragma once

#include "common/SignalVector2.hpp"
#include "controllers/taggedusers/TaggedUser.hpp"

namespace chatterino {

class TaggedUsersModel;

class TaggedUsersController
{
public:
    TaggedUsersController();

    SortedSignalVector<TaggedUser, std::less<TaggedUser>> users;

    TaggedUsersModel *createModel(QObject *parent = nullptr);
};

}  // namespace chatterino
