#include "util/IncognitoBrowser.hpp"
#ifdef USEWINSDK
#    include "util/WindowsHelper.hpp"
#endif

#include <QProcess>
#include <QRegularExpression>
#include <QVariant>

namespace {

using namespace chatterino;

#ifdef USEWINSDK
QString injectPrivateSwitch(QString command)
{
    // list of command line switches to turn on private browsing in browsers
    static auto switches = std::vector<std::pair<QString, QString>>{
        {"firefox", "-private-window"},  {"librewolf", "-private-window"},
        {"waterfox", "-private-window"}, {"icecat", "-private-window"},
        {"chrome", "-incognito"},        {"vivaldi", "-incognito"},
        {"opera", "-newprivatetab"},     {"opera\\\\launcher", "--private"},
        {"iexplore", "-private"},        {"msedge", "-inprivate"},
    };

    // transform into regex and replacement string
    std::vector<std::pair<QRegularExpression, QString>> replacers;
    for (const auto &switch_ : switches)
    {
        replacers.emplace_back(
            QRegularExpression("(" + switch_.first + "\\.exe\"?).*",
                               QRegularExpression::CaseInsensitiveOption),
            "\\1 " + switch_.second);
    }

    // try to find matching regex and apply it
    for (const auto &replacement : replacers)
    {
        if (replacement.first.match(command).hasMatch())
        {
            command.replace(replacement.first, replacement.second);
            return command;
        }
    }

    // couldn't match any browser -> unknown browser
    return QString();
}

QString getCommand()
{
    // get default browser start command, by protocol if possible, falling back to extension if not
    QString command =
        getAssociatedCommand(AssociationQueryType::Protocol, L"http");

    if (command.isNull())
    {
        // failed to fetch default browser by protocol, try by file extension instead
        command =
            getAssociatedCommand(AssociationQueryType::FileExtension, L".html");
    }

    if (command.isNull())
    {
        // also try the equivalent .htm extension
        command =
            getAssociatedCommand(AssociationQueryType::FileExtension, L".htm");
    }

    if (command.isNull())
    {
        // failed to find browser command
        return QString();
    }

    // inject switch to enable private browsing
    command = injectPrivateSwitch(command);
    if (command.isNull())
    {
        return QString();
    }

    return command;
}
#endif

}  // namespace

namespace chatterino {

bool supportsIncognitoLinks()
{
#ifdef USEWINSDK
    return !getCommand().isNull();
#else
    return false;
#endif
}

bool openLinkIncognito(const QString &link)
{
#ifdef USEWINSDK
    auto command = getCommand();

    // TODO: split command into program path and incognito argument
    return QProcess::startDetached(command, {link});
#else
    return false;
#endif
}

}  // namespace chatterino
