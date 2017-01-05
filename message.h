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
    Message(const IrcPrivateMessage& ircMessage, const Channel& Channel);

    bool canHighlightTab() {
        return m_highlightTab;
    }

    QString timeoutUser() {
        return m_timeoutUser;
    }

    int timeoutCount() {
        return m_timeoutCount;
    }

    QString userName() {
        return m_userName;
    }

    QString displayName() {
        return m_displayName;
    }

    QList<Word> words() {
        return m_words;
    }

    bool disabled() {
        return m_disabled;
    }

private:
    static LazyLoadedImage* badgeStaff;
    static LazyLoadedImage* badgeAdmin;
    static LazyLoadedImage* badgeGlobalmod;
    static LazyLoadedImage* badgeModerator;
    static LazyLoadedImage* badgeTurbo;
    static LazyLoadedImage* badgeBroadcaster;
    static LazyLoadedImage* badgePremium;

    bool m_highlightTab = false;
    QString m_timeoutUser = "";
    int m_timeoutCount = 0;
    bool m_disabled = false;
    std::chrono::time_point<std::chrono::system_clock> m_parseTime;

    QString m_userName = "";
    QString m_displayName = "";

    QList<Word> m_words;
};

#endif // MESSAGE_H
