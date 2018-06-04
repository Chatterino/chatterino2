#include "ircconnection2.hpp"

namespace chatterino {
namespace providers {
namespace irc {

IrcConnection::IrcConnection(QObject *parent)
    : Communi::IrcConnection(parent)
{
}

}  // namespace irc
}  // namespace providers
}  // namespace chatterino
