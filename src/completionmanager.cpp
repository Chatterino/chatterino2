#include "completionmanager.hpp"
#include "common.hpp"
#include "emotemanager.hpp"

namespace chatterino {

void CompletionModel::addString(const std::string &str)
{
    // Always add a space at the end of completions
    this->emotes.push_back(qS(str) + " ");
}

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
            model->addString(emoteName);
        }
    }

    std::vector<std::string> &bttvGlobalEmoteCodes = this->emoteManager.bttvGlobalEmoteCodes;
    for (const auto &m : bttvGlobalEmoteCodes) {
        model->addString(m);
    }

    std::vector<std::string> &ffzGlobalEmoteCodes = this->emoteManager.ffzGlobalEmoteCodes;
    for (const auto &m : ffzGlobalEmoteCodes) {
        model->addString(m);
    }

    std::vector<std::string> &bttvChannelEmoteCodes =
        this->emoteManager.bttvChannelEmoteCodes[channelName];
    for (const auto &m : bttvChannelEmoteCodes) {
        model->addString(m);
    }

    std::vector<std::string> &ffzChannelEmoteCodes =
        this->emoteManager.ffzChannelEmoteCodes[channelName];
    for (const auto &m : ffzChannelEmoteCodes) {
        model->addString(m);
    }

    const auto &emojiShortCodes = this->emoteManager.emojiShortCodes;
    for (const auto &m : emojiShortCodes) {
        model->addString(":" + m + ":");
    }
}

}  // namespace chatterino
