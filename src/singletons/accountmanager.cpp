#include "singletons/accountmanager.hpp"

namespace chatterino {
namespace singletons {

void AccountManager::load()
{
    this->Twitch.load();
}

}  // namespace singletons
}  // namespace chatterino
