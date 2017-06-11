#include "messages/message.hpp"
#include "channel.hpp"
#include "colorscheme.hpp"
#include "emojis.hpp"
#include "emotemanager.hpp"
#include "fontmanager.hpp"
#include "ircmanager.hpp"
#include "messages/link.hpp"
#include "resources.hpp"
#include "settingsmanager.hpp"

#include <QColor>
#include <QObjectUserData>
#include <QStringList>

#include <ctime>
#include <list>
#include <tuple>

namespace chatterino {
namespace messages {

Message::Message(const QString &text)
    : _words()
    , _text(text)
{
    _words.push_back(
        Word(text, Word::Text, ColorScheme::getInstance().SystemMessageColor, text, QString()));
}

Message::Message(const QString &text, const std::vector<Word> &words)
    : _words(words)
    , _text(text)
{
}

bool Message::getCanHighlightTab() const
{
    return _highlightTab;
}

const QString &Message::getTimeoutUser() const
{
    return _timeoutUser;
}

int Message::getTimeoutCount() const
{
    return _timeoutCount;
}

const QString &Message::getUserName() const
{
    return _userName;
}

const QString &Message::getDisplayName() const
{
    return _displayName;
}

const QString &Message::getContent() const
{
    return _content;
}

const std::chrono::time_point<std::chrono::system_clock> &Message::getParseTime() const
{
    return _parseTime;
}

std::vector<Word> &Message::getWords()
{
    return _words;
}

bool Message::isDisabled() const
{
    return _isDisabled;
}

const QString &Message::getId() const
{
    return _id;
}

}  // namespace messages
}  // namespace chatterino
