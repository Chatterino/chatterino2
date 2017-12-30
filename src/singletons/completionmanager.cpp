#include "singletons/completionmanager.hpp"
#include "common.hpp"
#include "debug/log.hpp"
#include "singletons/channelmanager.hpp"
#include "singletons/emotemanager.hpp"

namespace chatterino {

CompletionManager &CompletionManager::getInstance()
{
    static CompletionManager instance;
    return instance;
}

CompletionModel *CompletionManager::createModel(const std::string &channelName)
{
    auto it = this->models.find(channelName);
    if (it != this->models.end()) {
        return it->second;
    }

    CompletionModel *ret = new CompletionModel(qS(channelName));
    this->models[channelName] = ret;

    return ret;
}

}  // namespace chatterino
