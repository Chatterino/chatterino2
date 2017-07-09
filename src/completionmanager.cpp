#include "completionmanager.hpp"

namespace chatterino {

CompletionManager::CompletionManager()
{
}

CompletionModel *CompletionManager::createModel(const std::string &channelName)
{
    CompletionModel *ret = new CompletionModel();

    return ret;
}

void CompletionManager::updateModel(CompletionModel *model, const std::string &channelName)
{
}

}  // namespace chatterino
