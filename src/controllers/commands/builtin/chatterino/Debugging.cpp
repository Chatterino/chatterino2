#include "controllers/commands/builtin/chatterino/Debugging.hpp"

#include "common/Channel.hpp"
#include "common/Literals.hpp"
#include "controllers/commands/CommandContext.hpp"
#include "messages/MessageBuilder.hpp"
#include "singletons/Theme.hpp"

#include <QLoggingCategory>
#include <QString>

namespace chatterino::commands {

using namespace literals;

QString setLoggingRules(const CommandContext &ctx)
{
    if (ctx.words.size() < 2)
    {
        ctx.channel->addMessage(makeSystemMessage(
            "Usage: /c2-set-logging-rules <rules...>. To enable debug logging "
            "for all categories from chatterino, use "
            "'chatterino.*.debug=true'. For the format on the rules, see "
            "https://doc.qt.io/qt-6/"
            "qloggingcategory.html#configuring-categories"));
        return {};
    }

    auto filterRules = ctx.words.mid(1).join('\n');

    QLoggingCategory::setFilterRules(filterRules);

    auto message =
        QStringLiteral("Updated filter rules to '%1'.").arg(filterRules);

    if (!qgetenv("QT_LOGGING_RULES").isEmpty())
    {
        message += QStringLiteral(
            " Warning: Logging rules were previously set by the "
            "QT_LOGGING_RULES environment variable. This might cause "
            "interference - see: "
            "https://doc.qt.io/qt-6/qloggingcategory.html#setFilterRules");
    }

    ctx.channel->addMessage(makeSystemMessage(message));
    return {};
}

QString toggleThemeReload(const CommandContext &ctx)
{
    if (getTheme()->isAutoReloading())
    {
        getTheme()->setAutoReload(false);
        ctx.channel->addMessage(
            makeSystemMessage(u"Disabled theme auto reloading."_s));
        return {};
    }

    getTheme()->setAutoReload(true);
    ctx.channel->addMessage(
        makeSystemMessage(u"Auto reloading theme every %1 ms."_s.arg(
            Theme::AUTO_RELOAD_INTERVAL_MS)));
    return {};
}

}  // namespace chatterino::commands
