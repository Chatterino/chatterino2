#include "controllers/commands/builtin/chatterino/Debugging.hpp"

#include "common/Channel.hpp"
#include "controllers/commands/CommandContext.hpp"
#include "messages/MessageBuilder.hpp"

#include <QLoggingCategory>
#include <QString>

namespace chatterino::commands {

QString setLoggingRules(const CommandContext &ctx)
{
    if (ctx.words.size() < 2)
    {
        ctx.channel->addMessage(makeSystemMessage(
            "Usage: /c2:loggingrules <rules...>. To enable debug logging for "
            "all "
            "categories from chatterino, use 'chatterino.*.debug=true'. For "
            "the format on the rules, see "
            "https://doc.qt.io/qt-6/"
            "qloggingcategory.html#configuring-categories"));
        return {};
    }

    QLoggingCategory::setFilterRules(
        QList(ctx.words.begin() + 1, ctx.words.end()).join('\n'));

    auto message = QStringLiteral("Updated filter rules.");

    if (!qgetenv("QT_LOGGING_RULES").isEmpty())
    {
        message += QStringLiteral(
            " Warning: Logging rules were previously set by the "
            "QT_LOGGING_RULES environment variable. This might cause "
            "interference - see: "
            "https://doc.qt.io/qt-6/qloggingcategory.html#setFilterRules");
    }

    ctx.channel->addMessage(makeSystemMessage(message));
    return "";
}

}  // namespace chatterino::commands
