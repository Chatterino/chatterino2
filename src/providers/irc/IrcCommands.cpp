#include "IrcCommands.hpp"

#include "messages/MessageBuilder.hpp"
#include "providers/irc/IrcChannel2.hpp"
#include "providers/irc/IrcServer.hpp"
#include "util/Overloaded.hpp"
#include "util/QStringHash.hpp"

namespace chatterino {

Outcome invokeIrcCommand(const QString &commandName, const QString &allParams,
                         IrcChannel &channel)
{
    if (!channel.server())
    {
        return Failure;
    }

    // STATIC MESSAGES
    static auto staticMessages = std::unordered_map<QString, QString>{
        {"join", "/join is not supported. Press ctrl+r to change the "
                 "channel. If required use /raw JOIN #channel."},
        {"part", "/part is not supported. Press ctrl+r to change the "
                 "channel. If required use /raw PART #channel."},
    };
    auto cmd = commandName.toLower();

    if (auto it = staticMessages.find(cmd); it != staticMessages.end())
    {
        channel.addMessage(makeSystemMessage(it->second));
        return Success;
    }

    // CUSTOM COMMANDS
    auto params = allParams.split(' ');
    auto paramsAfter = [&](int i) { return params.mid(i + 1).join(' '); };

    auto sendRaw = [&](QString str) { channel.server()->sendRawMessage(str); };

    if (cmd == "msg")
    {
        sendRaw("PRIVMSG " + params[0] + " :" + paramsAfter(0));
    }
    else if (cmd == "away")
    {
        sendRaw("AWAY" + params[0] + " :" + paramsAfter(0));
    }
    else if (cmd == "knock")
    {
        sendRaw("KNOCK #" + params[0] + " " + paramsAfter(0));
    }
    else if (cmd == "kick")
    {
        if (paramsAfter(1).isEmpty())
            sendRaw("KICK " + params[0] + " " + params[1]);
        else
            sendRaw("KICK " + params[0] + " " + params[1] + " :" +
                    paramsAfter(1));
    }
    else if (cmd == "wallops")
    {
        sendRaw("WALLOPS :" + allParams);
    }
    else if (cmd == "raw")
    {
        sendRaw(allParams);
    }
    else
    {
        sendRaw(cmd.toUpper() + " " + allParams);
    }

    return Success;
}

}  // namespace chatterino
