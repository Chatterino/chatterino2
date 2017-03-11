#ifndef MESSAGE_H
#define MESSAGE_H

#include "messages/word.h"
#include "messages/wordpart.h"

#include <IrcMessage>
#include <QVector>

#include <chrono>

namespace chatterino {

class Channel;

namespace messages {

class Message
{
public:
    Message(const QString &text);
    Message(const IrcPrivateMessage &ircMessage, Channel &channel,
            bool enablePingSound = true, bool isReceivedWhisper = false,
            bool isSentWhisper = false, bool includeChannel = false);

    ~Message()
    {
    }

    bool
    getCanHighlightTab() const
    {
        return highlightTab;
    }

    const QString &
    getTimeoutUser() const
    {
        return timeoutUser;
    }

    int
    getTimeoutCount() const
    {
        return timeoutCount;
    }

    const QString &
    getUserName() const
    {
        return userName;
    }

    const QString &
    getDisplayName() const
    {
        return displayName;
    }

    inline const QString &
    getContent() const
    {
        return this->content;
    }

    inline const std::chrono::time_point<std::chrono::system_clock> &
    getParseTime() const
    {
        return this->parseTime;
    }

    std::vector<Word> &
    getWords()
    {
        return words;
    }

    bool
    getDisabled() const
    {
        return disabled;
    }

    const QString &
    getId() const
    {
        return id;
    }

private:
    static LazyLoadedImage *badgeStaff;
    static LazyLoadedImage *badgeAdmin;
    static LazyLoadedImage *badgeGlobalmod;
    static LazyLoadedImage *badgeModerator;
    static LazyLoadedImage *badgeTurbo;
    static LazyLoadedImage *badgeBroadcaster;
    static LazyLoadedImage *badgePremium;

    static QRegularExpression *cheerRegex;

    bool highlightTab = false;
    QString timeoutUser = "";
    int timeoutCount = 0;
    bool disabled = false;
    std::chrono::time_point<std::chrono::system_clock> parseTime;

    QString userName = "";
    QString displayName = "";
    QString content;
    QString id = "";

    std::vector<Word> words;

    static QString matchLink(const QString &string);

    static bool sortTwitchEmotes(
        const std::pair<long int, LazyLoadedImage *> &a,
        const std::pair<long int, LazyLoadedImage *> &b);
};

}  // namespace messages
}  // namespace chatterino

#endif  // MESSAGE_H
