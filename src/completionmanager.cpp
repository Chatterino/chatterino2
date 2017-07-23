#include "completionmanager.hpp"
#include "common.hpp"
#include "emotemanager.hpp"

namespace chatterino {

CompletionManager::CompletionManager(EmoteManager &_emoteManager)
    : emoteManager(_emoteManager)
{
}

CompletionModel *CompletionManager::createModel(const std::string &channelName)
{
    CompletionModel *ret = new CompletionModel();

    this->updateModel(ret, channelName);

    this->emoteManager.bttvGlobalEmoteCodes.updated.connect([=]() {
        this->updateModel(ret, channelName);  //
    });

    this->emoteManager.ffzGlobalEmoteCodes.updated.connect([=]() {
        this->updateModel(ret, channelName);  //
    });

    this->emoteManager.bttvChannelEmoteCodes[channelName].updated.connect([=]() {
        this->updateModel(ret, channelName);  //
    });

    this->emoteManager.ffzChannelEmoteCodes[channelName].updated.connect([=]() {
        this->updateModel(ret, channelName);  //
    });

    return ret;
}

void CompletionManager::updateModel(CompletionModel *model, const std::string &channelName)
{
    model->emotes.clear();

    for (const auto &m : this->emoteManager.twitchAccountEmotes) {
        for (const auto &emoteName : m.second.emoteCodes) {
            model->emotes.push_back(qS(emoteName));
        }
    }

    std::vector<std::string> &bttvGlobalEmoteCodes = this->emoteManager.bttvGlobalEmoteCodes;
    for (const auto &m : bttvGlobalEmoteCodes) {
        model->emotes.push_back(qS(m));
    }

    std::vector<std::string> &ffzGlobalEmoteCodes = this->emoteManager.ffzGlobalEmoteCodes;
    for (const auto &m : ffzGlobalEmoteCodes) {
        model->emotes.push_back(qS(m));
    }

    std::vector<std::string> &bttvChannelEmoteCodes =
        this->emoteManager.bttvChannelEmoteCodes[channelName];
    for (const auto &m : bttvChannelEmoteCodes) {
        model->emotes.push_back(qS(m));
    }

    std::vector<std::string> &ffzChannelEmoteCodes =
        this->emoteManager.ffzChannelEmoteCodes[channelName];
    for (const auto &m : ffzChannelEmoteCodes) {
        model->emotes.push_back(qS(m));
    }
}

}  // namespace chatterino
