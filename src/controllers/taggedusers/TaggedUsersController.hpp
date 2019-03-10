#pragma once

#include "common/Singleton.hpp"

#include "common/SignalVector.hpp"
#include "controllers/taggedusers/TaggedUser.hpp"

namespace chatterino
{
    class TaggedUsersModel;

    class TaggedUsersController final : public Singleton
    {
    public:
        TaggedUsersController();

        SortedSignalVector<TaggedUser, std::less<TaggedUser>> users;

        TaggedUsersModel* createModel(QObject* parent = nullptr);
    };

}  // namespace chatterino
