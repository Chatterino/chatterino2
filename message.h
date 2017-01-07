#ifndef MESSAGE_H
#define MESSAGE_H

#include "IrcMessage"
#include "word.h"
#include "chrono"
#include "channel.h"

class Message
{
public:
//    enum Badges : char {
//        None = 0,
//        Mod = 1,
//        Turbo = 2,
//        Sub = 4,
//        Staff = 8,
//        GlobalMod = 16,
//        Admin = 32,
//        Broadcaster = 64,
//    };

    Message(const QString& text);
    Message(const IrcPrivateMessage& ircMessage, const Channel& Channel, bool enablePingSound = true,
            bool isReceivedWhisper = false, bool isSentWhisper = false, bool includeChannel = false);

    bool canHighlightTab() const {
        return m_highlightTab;
    }

    const QString& timeoutUser() const {
        return m_timeoutUser;
    }

    int timeoutCount() const {
        return m_timeoutCount;
    }

    const QString& userName() const {
        return m_userName;
    }

    const QString& displayName() const {
        return m_displayName;
    }

    QList<Word> words() const {
        return m_words;
    }

    bool disabled() const {
        return m_disabled;
    }

    const QString& id() const {
        return m_id;
    }

private:
    static LazyLoadedImage* badgeStaff;
    static LazyLoadedImage* badgeAdmin;
    static LazyLoadedImage* badgeGlobalmod;
    static LazyLoadedImage* badgeModerator;
    static LazyLoadedImage* badgeTurbo;
    static LazyLoadedImage* badgeBroadcaster;
    static LazyLoadedImage* badgePremium;

    static QRegularExpression* cheerRegex;

    bool m_highlightTab = false;
    QString m_timeoutUser = "";
    int m_timeoutCount = 0;
    bool m_disabled = false;
    std::chrono::time_point<std::chrono::system_clock> m_parseTime;

    QString m_userName = "";
    QString m_displayName = "";
    QString m_id = "";

    QList<Word> m_words;

    static QString matchLink(const QString& string);

    static bool sortTwitchEmotes(const std::pair<long int, LazyLoadedImage*>& a, const std::pair<long int, LazyLoadedImage*>& b);
};

#endif // MESSAGE_H
