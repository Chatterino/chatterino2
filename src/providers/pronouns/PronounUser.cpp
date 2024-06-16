#include "providers/pronouns/PronounUser.hpp"

namespace chatterino {

PronounUser::PronounUser()
    : id{}
    , username{}
{
}
PronounUser::PronounUser(std::string id, std::string username)
    : id{id}
    , username{username}
{
}

}  // namespace chatterino
