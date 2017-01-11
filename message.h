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
        if (m_wordParts != NULL) {
            delete m_wordParts;
        }
    }

    bool
    canHighlightTab() const
    {
        return m_highlightTab;
    }

    const QString &
    timeoutUser() const
    {
        return m_timeoutUser;
    }

    int
    timeoutCount() const
    {
        return m_timeoutCount;
    }

    const QString &
    userName() const
    {
        return m_userName;
    }

    const QString &
    displayName() const
    {
        return m_displayName;
    }

    const std::vector<Word>
    words() const
    {
        return m_words;
    }

    const std::list<WordPart>
    wordParts() const
    {
        return *m_wordParts;
    }

    bool
    disabled() const
    {
        return m_disabled;
    }

    const QString &
    id() const
    {
        return m_id;
    }

    int
    height() const
    {
        return m_height;
    }

    bool layout(int width, bool enableEmoteMargins = true);

    void
    requestRelayout()
    {
        m_relayoutRequested = true;
    }
    void
    requestTextRecalculation()
    {
        m_recalculateText = true;
    }
    void
    requestImageRecalculation()
    {
        m_recalculateImages = true;
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

    bool m_highlightTab = false;
    QString m_timeoutUser = "";
    int m_timeoutCount = 0;
    bool m_disabled = false;
    std::chrono::time_point<std::chrono::system_clock> m_parseTime;

    QString m_userName = "";
    QString m_displayName = "";
    QString m_id = "";

    int m_height = 0;

    std::vector<Word> m_words;
    std::list<WordPart> *m_wordParts;

    long m_currentLayoutWidth = -1;
    bool m_relayoutRequested = true;
    bool m_recalculateText = true;
    bool m_recalculateImages = true;

    static QString matchLink(const QString &string);

    static bool sortTwitchEmotes(
        const std::pair<long int, LazyLoadedImage *> &a,
        const std::pair<long int, LazyLoadedImage *> &b);
};

#endif  // MESSAGE_H
