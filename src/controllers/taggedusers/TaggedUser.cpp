#include "TaggedUser.hpp"

#include <tuple>

namespace chatterino
{
    TaggedUser::TaggedUser(
        ProviderId provider, const QString& name, const QString& id)
        : providerId_(provider)
        , name_(name)
        , id_(id)
    {
    }

    bool TaggedUser::operator<(const TaggedUser& other) const
    {
        return std::tie(this->providerId_, this->name_, this->id_) <
               std::tie(other.providerId_, other.name_, other.id_);
    }

    ProviderId TaggedUser::getProviderId() const
    {
        return this->providerId_;
    }

    QString TaggedUser::getName() const
    {
        return this->name_;
    }

    QString TaggedUser::getId() const
    {
        return this->id_;
    }

}  // namespace chatterino
