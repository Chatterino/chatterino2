#include "singletons/completionmanager.hpp"
#include "common.hpp"
#include "debug/log.hpp"
#include "singletons/channelmanager.hpp"
#include "singletons/emotemanager.hpp"

namespace chatterino {
namespace singletons {

CompletionManager &CompletionManager::getInstance()
{
    static CompletionManager instance;
    return instance;
}

CompletionModel *CompletionManager::createModel(const QString &channelName)
{
    auto it = this->models.find(channelName);
    if (it != this->models.end()) {
        return it->second;
    }

    CompletionModel *ret = new CompletionModel(channelName);
    this->models[channelName] = ret;

    return ret;
}

}  // namespace singletons
}  // namespace chatterino
