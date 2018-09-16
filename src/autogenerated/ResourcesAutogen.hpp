#include <QPixmap>
#include "common/Singleton.hpp"

namespace chatterino {

class Resources2 : public Singleton {
public:
    Resources2();

    struct {
        QPixmap fourtf;
        QPixmap pajlada;
    } avatars;
    struct {
        QPixmap addSplitDark;
        QPixmap ban;
        QPixmap banRed;
        QPixmap menuDark;
        QPixmap menuLight;
        QPixmap mod;
        QPixmap modModeDisabled;
        QPixmap modModeDisabled2;
        QPixmap modModeEnabled;
        QPixmap modModeEnabled2;
        QPixmap timeout;
        QPixmap unban;
        QPixmap unmod;
        QPixmap update;
        QPixmap updateError;
    } buttons;
    QPixmap error;
    QPixmap icon;
    QPixmap pajaDank;
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
    struct {
        QPixmap admin;
        QPixmap broadcaster;
        QPixmap cheer1;
        QPixmap globalmod;
        QPixmap moderator;
        QPixmap prime;
        QPixmap staff;
        QPixmap subscriber;
        QPixmap turbo;
        QPixmap verified;
    } twitch;
};

}  // namespace chatterino