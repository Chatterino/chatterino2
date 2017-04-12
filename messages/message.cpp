#include "messages/message.h"
#include "channel.h"
#include "colorscheme.h"
#include "emojis.h"
#include "emotemanager.h"
#include "fontmanager.h"
#include "ircmanager.h"
#include "messages/link.h"
#include "qcolor.h"
#include "resources.h"
#include "settingsmanager.h"

#include <QObjectUserData>
#include <QStringList>
#include <ctime>
#include <list>
#include <tuple>

namespace chatterino {
namespace messages {

Message::Message(const QString &text)
    : _words()
{
    _words.push_back(
        Word(text, Word::Text, ColorScheme::getInstance().SystemMessageColor, text, QString()));
}

Message::Message(const std::vector<Word> &words)
    : _words(words)
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
