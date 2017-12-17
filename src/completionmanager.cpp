#include "completionmanager.hpp"
#include "common.hpp"
#include "emotemanager.hpp"

namespace chatterino {

void CompletionModel::addString(const std::string &str)
{
    // Always add a space at the end of completions
    this->emotes.push_back(qS(str) + " ");
}

CompletionModel *CompletionManager::createModel(const std::string &channelName)
{
    CompletionModel *ret = new CompletionModel();
    auto &emoteManager = EmoteManager::getInstance();

    this->updateModel(ret, channelName);

    emoteManager.bttvGlobalEmoteCodes.updated.connect([=]() {
        this->updateModel(ret, channelName);  //
    });

    emoteManager.ffzGlobalEmoteCodes.updated.connect([=]() {
        this->updateModel(ret, channelName);  //
    });

    emoteManager.bttvChannelEmoteCodes[channelName].updated.connect([=]() {
        this->updateModel(ret, channelName);  //
    });

    emoteManager.ffzChannelEmoteCodes[channelName].updated.connect([=]() {
        this->updateModel(ret, channelName);  //
    });

    return ret;
}

void CompletionManager::updateModel(CompletionModel *model, const std::string &channelName)
{
    auto &emoteManager = EmoteManager::getInstance();

    model->emotes.clear();

    for (const auto &m : emoteManager.twitchAccountEmotes) {
        for (const auto &emoteName : m.second.emoteCodes) {
            model->addString(emoteName);
        }
    }

    std::vector<std::string> &bttvGlobalEmoteCodes = emoteManager.bttvGlobalEmoteCodes;
    for (const auto &m : bttvGlobalEmoteCodes) {
        model->addString(m);
    }

    std::vector<std::string> &ffzGlobalEmoteCodes = emoteManager.ffzGlobalEmoteCodes;
    for (const auto &m : ffzGlobalEmoteCodes) {
        model->addString(m);
    }

    std::vector<std::string> &bttvChannelEmoteCodes =
        emoteManager.bttvChannelEmoteCodes[channelName];
    for (const auto &m : bttvChannelEmoteCodes) {
        model->addString(m);
    }

    std::vector<std::string> &ffzChannelEmoteCodes = emoteManager.ffzChannelEmoteCodes[channelName];
    for (const auto &m : ffzChannelEmoteCodes) {
        model->addString(m);
    }

    const auto &emojiShortCodes = emoteManager.emojiShortCodes;
    for (const auto &m : emojiShortCodes) {
        model->addString(":" + m + ":");
    }
}

}  // namespace chatterino
