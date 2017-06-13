#pragma once

#include "messages/message.hpp"
#include "messages/messageparseargs.hpp"
#include "messages/word.hpp"
#include "messages/wordpart.hpp"

#include <IrcMessage>
#include <QVector>

#include <chrono>
#include <memory>

namespace chatterino {

class Channel;

namespace messages {
class Message;

typedef std::shared_ptr<Message> SharedMessage;

class Message
{
public:
    explicit Message(const QString &text);
    explicit Message(const QString &text, const std::vector<messages::Word> &words);

    bool getCanHighlightTab() const;
    const QString &getTimeoutUser() const;
    int getTimeoutCount() const;
    const QString &getUserName() const;
    const QString &getDisplayName() const;
    const QString &getContent() const;
    const std::chrono::time_point<std::chrono::system_clock> &getParseTime() const;
    std::vector<Word> &getWords();
    bool isDisabled() const;
    const QString &getId() const;

    const QString _text;

private:
    static LazyLoadedImage *badgeStaff;
    static LazyLoadedImage *badgeAdmin;
    static LazyLoadedImage *badgeGlobalmod;
    static LazyLoadedImage *badgeModerator;
    static LazyLoadedImage *badgeTurbo;
    static LazyLoadedImage *badgeBroadcaster;
    static LazyLoadedImage *badgePremium;

    static QRegularExpression *cheerRegex;

    bool _highlightTab = false;
    QString _timeoutUser = "";
    int _timeoutCount = 0;
    bool _isDisabled = false;
    std::chrono::time_point<std::chrono::system_clock> _parseTime;

    QString _userName = "";
    QString _displayName = "";
    QString _content;
    QString _id = "";

    std::vector<Word> _words;
};

}  // namespace messages
}  // namespace chatterino
