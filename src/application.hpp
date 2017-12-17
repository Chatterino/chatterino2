#pragma once

#include "channelmanager.hpp"
#include "colorscheme.hpp"
#include "completionmanager.hpp"
#include "emotemanager.hpp"
#include "ircmanager.hpp"
#include "resources.hpp"
#include "windowmanager.hpp"

#include <QApplication>

namespace chatterino {

class Application
{
public:
    Application();
    ~Application();

    int run(QApplication &qtApp);

    WindowManager windowManager;
    ColorScheme colorScheme;
    Resources resources;
    ChannelManager channelManager;
    IrcManager ircManager;
};

}  // namespace chatterino
