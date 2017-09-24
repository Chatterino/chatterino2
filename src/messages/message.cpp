#include "messages/message.hpp"
#include "channel.hpp"
#include "colorscheme.hpp"
#include "emojis.hpp"
#include "emotemanager.hpp"
#include "fontmanager.hpp"
#include "ircmanager.hpp"
#include "messages/link.hpp"
#include "resources.hpp"

#include <ctime>
#include <list>
#include <tuple>

namespace chatterino {
namespace messages {

bool Message::getCanHighlightTab() const
{
    return this->highlightTab;
}

void Message::setHighlight(bool value)
{
    this->highlightTab = value;
}

const QString &Message::getTimeoutUser() const
{
    return this->timeoutUser;
}

int Message::getTimeoutCount() const
{
    return this->timeoutCount;
}

const QString &Message::getUserName() const
{
    return this->userName;
}

const QString &Message::getDisplayName() const
{
    return this->displayName;
}

const QString &Message::getContent() const
{
    return this->content;
}

const std::chrono::time_point<std::chrono::system_clock> &Message::getParseTime() const
{
    return this->parseTime;
}

std::vector<Word> &Message::getWords()
{
    return this->words;
}

bool Message::isDisabled() const
{
    return this->disabled;
}

const QString &Message::getId() const
{
    return this->id;
}

}  // namespace messages
}  // namespace chatterino
