#ifndef MESSAGE_H
#define MESSAGE_H

#include "channel.h"
#include "word.h"
#include "wordpart.h"

#include <IrcMessage>
#include <QVector>
#include <chrono>

class Message
{
public:
    Message(const QString &text);
    Message(const IrcPrivateMessage &ircMessage, const Channel &Channel,
            bool enablePingSound = true, bool isReceivedWhisper = false,
            bool isSentWhisper = false, bool includeChannel = false);

    ~Message()
    {
        if (wordParts != NULL) {
            delete wordParts;
        }
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

    const std::vector<Word>
    getWords() const
    {
        return words;
    }

    const std::vector<WordPart>
    getWordParts() const
    {
        return *wordParts;
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

    int
    getHeight() const
    {
        return height;
    }

    bool layout(int width, bool enableEmoteMargins = true);

    void
    requestRelayout()
    {
        relayoutRequested = true;
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
    QString id = "";

    int height = 0;

    std::vector<Word> words;
    std::vector<WordPart> *wordParts;

    long currentLayoutWidth = -1;
    bool relayoutRequested = true;
    int fontGeneration = -1;
    int emoteGeneration = -1;

    static QString matchLink(const QString &string);

    static bool sortTwitchEmotes(
        const std::pair<long int, LazyLoadedImage *> &a,
        const std::pair<long int, LazyLoadedImage *> &b);
};

#endif  // MESSAGE_H
