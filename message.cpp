#include "message.h"
#include "qcolor.h"
#include "colorscheme.h"
#include "emotes.h"

#include <ctime>

LazyLoadedImage* Message::badgeStaff       = new LazyLoadedImage(new QImage(":/images/staff_bg.png"));
LazyLoadedImage* Message::badgeAdmin       = new LazyLoadedImage(new QImage(":/images/admin_bg.png"));
LazyLoadedImage* Message::badgeModerator   = new LazyLoadedImage(new QImage(":/images/moderator_bg.png"));
LazyLoadedImage* Message::badgeGlobalmod   = new LazyLoadedImage(new QImage(":/images/globalmod_bg.png"));
LazyLoadedImage* Message::badgeTurbo       = new LazyLoadedImage(new QImage(":/images/turbo_bg.png"));
LazyLoadedImage* Message::badgeBroadcaster = new LazyLoadedImage(new QImage(":/images/broadcaster_bg.png"));
LazyLoadedImage* Message::badgePremium     = new LazyLoadedImage(new QImage(":/images/twitchprime_bg.png"));

Message::Message(const QString &text)
{

}

Message::Message(const IrcPrivateMessage& ircMessage, const Channel& channel, bool enablePingSound,
                 bool isReceivedWhisper, bool isSentWhisper, bool includeChannel )
{
    m_parseTime = std::chrono::system_clock::now();

    auto words = new QList<Word>();

    auto iterator = ircMessage.tags().find("id");

    if (iterator != ircMessage.tags().end())
    {
        m_id = iterator.value().toString();
    }

    // timestamps
    iterator = ircMessage.tags().find("tmi-sent-ts");
    std::time_t time = std::time(NULL);

    if (iterator != ircMessage.tags().end())
    {
        time = strtoll(iterator.value().toString().toStdString().c_str(), NULL, 10);
    }

    char timeStampBuffer[69];

    strftime(timeStampBuffer, 69, "%H:%M", localtime(&time));
    QString timestamp = QString(timeStampBuffer);

    strftime(timeStampBuffer, 69, "%H:%M:%S", localtime(&time));
    QString timestampWithSeconds = QString(timeStampBuffer);

    words->append(Word(timestamp, Word::TimestampNoSeconds, ColorScheme::getInstance().SystemMessageColor, QString(), QString()));
    words->append(Word(timestampWithSeconds, Word::TimestampWithSeconds, ColorScheme::getInstance().SystemMessageColor, QString(), QString()));

    // color
    QColor usernameColor = ColorScheme::getInstance().SystemMessageColor;

    iterator = ircMessage.tags().find("color");
    if (iterator != ircMessage.tags().end())
    {
         usernameColor = QColor(iterator.value().toString());
    }

    // channel name
    if (includeChannel)
    {
        QString channelName("#" + channel.name());
       words->append(Word(channelName, Word::Misc, QString(channelName), QString(), Link(Link::ShowMessage, channel.name() + "\n" + m_id)));
    }

    // username
    m_userName = ircMessage.account();

    if (m_userName.isEmpty())
    {
        auto iterator = ircMessage.tags().find("login");

        if (iterator != ircMessage.tags().end())
        {
            m_userName = iterator.value().toString();
        }
    }

    // display name
    QString displayName;

    iterator = ircMessage.tags().find("display-name");
    if (iterator == ircMessage.tags().end()) {
        displayName = ircMessage.account();
    }
    else {
        displayName = iterator.value().toString();
    }

    bool hasLocalizedName = QString::compare(displayName, ircMessage.account()) == 0;
    QString userDisplayString = displayName + (hasLocalizedName ? (" (" + ircMessage.account() + ")") : QString());

    words->append(Word(userDisplayString, Word::Username, usernameColor, userDisplayString, QString()));

    // highlights
#pragma message WARN("xD")

    // bits
    QString bits = "";

    iterator = ircMessage.tags().find("bits");
    if (iterator != ircMessage.tags().end())
    {
         bits = iterator.value().toString();
    }

    // badges
    iterator = ircMessage.tags().find("badges");

    if (iterator != ircMessage.tags().end())
    {
        auto badges = iterator.value().toString().split(',');

        for (QString badge : badges)
        {
            if (badge.startsWith("bits/"))
            {
                long long int cheer = strtoll(badge.mid(5).toStdString().c_str(), NULL, 10);
                words->append(Word(Emotes::getCheerBadge(cheer), Word::BadgeCheer, QString(), QString("Twitch Cheer" + QString::number(cheer))));
            }
            else if (badge == "staff/1")
            {
                words->append(Word(badgeStaff, Word::BadgeStaff, QString(), QString("Twitch Staff")));
            }
            else if (badge == "admin/1")
            {
                words->append(Word(badgeAdmin, Word::BadgeAdmin, QString(), QString("Twitch Admin")));
            }
            else if (badge == "global_mod/1")
            {
                words->append(Word(badgeGlobalmod, Word::BadgeGlobalMod, QString(), QString("Global Moderator")));
            }
            else if (badge == "moderator/1")
            {
#warning "xD"
                words->append(Word(badgeTurbo, Word::BadgeModerator, QString(), QString("Channel Moderator"))); // custom badge
            }
            else if (badge == "turbo/1")
            {
                words->append(Word(badgeStaff, Word::BadgeTurbo, QString(), QString("Turbo Subscriber")));
            }
            else if (badge == "broadcaster/1")
            {
                words->append(Word(badgeBroadcaster, Word::BadgeBroadcaster, QString(), QString("Channel Broadcaster")));
            }
            else if (badge == "premium/1")
            {
                words->append(Word(badgePremium, Word::BadgePremium, QString(), QString("Twitch Prime")));
            }
        }
    }


}
