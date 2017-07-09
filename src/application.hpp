#pragma once

#include "channelmanager.hpp"
#include "colorscheme.hpp"
#include "emotemanager.hpp"
#include "ircmanager.hpp"
#include "messagefactory.hpp"
#include "resources.hpp"
#include "windowmanager.hpp"
#include "completionmanager.hpp"

#include <QApplication>

namespace chatterino {

class Application
{
public:
    Application();
    ~Application();

    int run(QApplication &qtApp);

    CompletionManager completionManager;
    WindowManager windowManager;
    ColorScheme colorScheme;
    EmoteManager emoteManager;
    Resources resources;
    ChannelManager channelManager;
    IrcManager ircManager;
    MessageFactory messageFactory;
};

}  // namespace chatterino
