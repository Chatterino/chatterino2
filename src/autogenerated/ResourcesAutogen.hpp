#include <QPixmap>
#include "common/Singleton.hpp"

namespace chatterino {

class Resources2 : public Singleton
{
public:
    Resources2();

    struct {
        QPixmap fourtf;
        QPixmap mm2pl;
        QPixmap pajlada;
        QPixmap zneix;
    } avatars;
    struct {
        QPixmap addSplit;
        QPixmap addSplitDark;
        QPixmap ban;
        QPixmap banRed;
        QPixmap copyDark;
        QPixmap copyDarkTheme;
        QPixmap copyLight;
        QPixmap menuDark;
        QPixmap menuLight;
        QPixmap mod;
        QPixmap modModeDisabled;
        QPixmap modModeDisabled2;
        QPixmap modModeEnabled;
        QPixmap modModeEnabled2;
        QPixmap search;
        QPixmap timeout;
        QPixmap trashCan;
        QPixmap unban;
        QPixmap unmod;
        QPixmap unvip;
        QPixmap update;
        QPixmap updateError;
        QPixmap viewersDark;
        QPixmap viewersLight;
        QPixmap vip;
    } buttons;
    QPixmap error;
    QPixmap icon;
    QPixmap pajaDank;
    struct {
        QPixmap downScroll;
        QPixmap neutralScroll;
        QPixmap upScroll;
    } scrolling;
    struct {
        QPixmap aboutlogo;
    } settings;
    struct {
        QPixmap down;
        QPixmap left;
        QPixmap move;
        QPixmap right;
        QPixmap up;
    } split;
    QPixmap streamerMode;
    struct {
        QPixmap admin;
        QPixmap automod;
        QPixmap broadcaster;
        QPixmap cheer1;
        QPixmap globalmod;
        QPixmap moderator;
        QPixmap prime;
        QPixmap staff;
        QPixmap subscriber;
        QPixmap turbo;
        QPixmap verified;
        QPixmap vip;
    } twitch;
};

}  // namespace chatterino