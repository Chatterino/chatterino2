#include "message.h"
#include "qcolor.h"
#include "colorscheme.h"
#include "emotes.h"

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

Message::Message(const IrcPrivateMessage& ircMessage, const Channel& Channel)
{
    m_parseTime = std::chrono::system_clock::now();

    auto words = new QList<Word>();

    // timestamps
    iterator = ircMessage.tags().find("tmi-sent-ts");
    time_t time = std::time(NULL);

    if (iterator != ircMessage.tags().end())
    {
        time = strtoll(iterator.value().toString().toStdString().c_str(), NULL, 10);
    }

    char timeStampBuffer[69];

    strftime(timeStampBuffer, 69, "%H:%M", localtime(&time));
    QString timestamp = QString(timeStampBuffer);

    strftime(timeStampBuffer, 69, "%H:%M:%S", localtime(&time));
    QString timestampWithSeconds = QString(timeStampBuffer);

    words->append(new Word(timestamp, Word::TimestampNoSeconds));
    words->append(new Word(timestampWithSeconds, Word::TimestampWithSeconds));

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

    auto iterator = ircMessage.tags().find("display-name");
    if (iterator == ircMessage.tags().end()) {
        displayName = m_userName;
    }
    else {
        displayName = iterator.value().toString();
    }

    // highlights
#warning "xD"

    // color
    QColor usernameColor = ColorScheme::getInstance().SystemMessageColor;

    iterator = ircMessage.tags().find("color");
    if (iterator != ircMessage.tags().end())
    {
         usernameColor = QColor(iterator.value().toString());
    }

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

            }
            else if (badge == "staff/1")
            {
                QString a("");
                QString b("Twitch Staff");
                words->append(*new Word(badgeStaff, Word::BadgeStaff, a, b));
            }
//            else if (badge == "admin/1")
//            {
//                words->append(*new Word(badgeAdmin, Word::BadgeAdmin, "", "Twitch Admin"));
//            }
//            else if (badge == "global_mod/1")
//            {
//                words->append(*new Word(badgeGlobalmod, Word::BadgeGlobalMod, "", "Global Moderator"));
//            }
//            else if (badge == "moderator/1")
//            {
//#warning "xD"
//                words->append(*new Word(badgeTurbo, Word::BadgeModerator, "", "Channel Moderator")); // custom badge
//            }
//            else if (badge == "turbo/1")
//            {
//                words->append(*new Word(badgeStaff, Word::BadgeTurbo, "", "Turbo Subscriber"));
//            }
//            else if (badge == "broadcaster/1")
//            {
//                words->append(*new Word(badgeBroadcaster, Word::BadgeBroadcaster, "", "Channel Broadcaster"));
//            }
//            else if (badge == "premium/1")
//            {
//                words->append(*new Word(badgeTwitchPrime, Word::BadgePremium, "", "Twitch Prime"));
//            }


//            case "staff/1":
//                Badges |= MessageBadges.Staff;
//                words.Add(new Word { Type = SpanType.Image, Value = GuiEngine.Current.GetImage(ImageType.BadgeStaff), Tooltip =  });
//                break;
//            case "admin/1":
//                Badges |= MessageBadges.Admin;
//                words.Add(new Word { Type = SpanType.Image, Value = GuiEngine.Current.GetImage(ImageType.BadgeAdmin), Tooltip = "Twitch Admin" });
//                break;
//            case "global_mod/1":
//                Badges |= MessageBadges.GlobalMod;
//                words.Add(new Word { Type = SpanType.Image, Value = GuiEngine.Current.GetImage(ImageType.BadgeGlobalmod), Tooltip = "Global Moderator" });
//                break;
//            case "moderator/1":
//                Badges |= MessageBadges.Mod;
//                if (channel.ModeratorBadge == null)
//                {
//                    words.Add(new Word { Type = SpanType.Image, Value = GuiEngine.Current.GetImage(ImageType.BadgeModerator), Tooltip = "Channel Moderator" });
//                }
//                else
//                {
//                    words.Add(new Word { Type = SpanType.LazyLoadedImage, Value = channel.ModeratorBadge, Tooltip = channel.ModeratorBadge.Tooltip });
//                }
//                break;
//            case "turbo/1":
//                Badges |= MessageBadges.Turbo;
//                words.Add(new Word { Type = SpanType.Image, Value = GuiEngine.Current.GetImage(ImageType.BadgeTurbo), Tooltip = "Turbo Subscriber" });
//                break;
//            case "broadcaster/1":
//                Badges |= MessageBadges.Broadcaster;
//                words.Add(new Word { Type = SpanType.Image, Value = GuiEngine.Current.GetImage(ImageType.BadgeBroadcaster), Tooltip = "Channel Broadcaster" });
//                break;
//            case "premium/1":
//                Badges |= MessageBadges.Broadcaster;
//                words.Add(new Word { Type = SpanType.Image, Value = GuiEngine.Current.GetImage(ImageType.BadgeTwitchPrime), Tooltip = "Twitch Prime" });
//                break;


//                 long long int cheer = strtoll(badge.mid(5).toStdString().c_str(), NULL, 10);

//            auto image = Emotes::getCheerImage(cheer, false);
//            auto imageAnimated = Emotes::getCheerImage(cheer, true);

//            words->append(*new Word(image, Word::Bits));
//            words->append(*new Word(imageAnimated, Word::BitsAnimated));

        }
    }
}
