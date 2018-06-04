#pragma once

#include <IrcConnection>

namespace chatterino {
namespace providers {
namespace irc {

class IrcConnection : public Communi::IrcConnection
{
public:
    IrcConnection(QObject *parent = nullptr);
};

}  // namespace irc
}  // namespace providers
}  // namespace chatterino
