#include "singletons/emotemanager.hpp"

#include "application.hpp"
#include "common.hpp"
#include "controllers/accounts/accountcontroller.hpp"

#include <QFile>

using namespace chatterino::providers::twitch;
using namespace chatterino::messages;

namespace chatterino {
namespace singletons {

void EmoteManager::initialize()
{
    getApp()->accounts->twitch.currentUserChanged.connect([this] {
        auto currentUser = getApp()->accounts->twitch.getCurrent();
        assert(currentUser);
        this->twitch.refresh(currentUser);
    });

    this->emojis.load();
    this->bttv.loadGlobalEmotes();
    this->ffz.loadGlobalEmotes();

    this->gifTimer.initialize();
}

util::EmoteMap &EmoteManager::getChatterinoEmotes()
{
    return _chatterinoEmotes;
}

util::EmoteData EmoteManager::getCheerImage(long long amount, bool animated)
{
    // TODO: fix this xD
    return util::EmoteData();
}

}  // namespace singletons
}  // namespace chatterino

#if 0
namespace chatterino {

void EmojiTest()
{
    auto &emoteManager = singletons::EmoteManager::getInstance();

    emoteManager.loadEmojis();

    {
        std::vector<std::tuple<util::EmoteData, QString>> dummy;

        // couple_mm 1f468-2764-1f468
        // "\154075\156150❤\154075\156150"
        // [0]            55357    0xd83d    QChar
        // [1]            56424    0xdc68    QChar
        // [2]    '❤'     10084    0x2764    QChar
        // [3]            55357    0xd83d    QChar
        // [4]            56424    0xdc68    QChar
        QString text = "👨❤👨";

        emoteManager.parseEmojis(dummy, text);

        assert(dummy.size() == 1);
    }

    {
        std::vector<std::tuple<util::EmoteData, QString>> dummy;

        // "✍\154074\157777"
        // [0]    '✍'     9997    0x270d    QChar
        // [1]            55356    0xd83c    QChar
        // [2]            57343    0xdfff    QChar
        QString text = "✍🏿";

        emoteManager.parseEmojis(dummy, text);

        assert(dummy.size() == 1);

        assert(std::get<0>(dummy[0]).isValid());
    }

    {
        std::vector<std::tuple<util::EmoteData, QString>> dummy;

        QString text = "✍";

        emoteManager.parseEmojis(dummy, text);

        assert(dummy.size() == 1);

        assert(std::get<0>(dummy[0]).isValid());
    }
}

}  // namespace chatterino
#endif
