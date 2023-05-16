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
            "Usage: /c2-set-logging-rules <rules...>. To enable debug logging "
            "for all categories from chatterino, use "
            "'chatterino.*.debug=true'. For the format on the rules, see "
            "https://doc.qt.io/qt-6/"
            "qloggingcategory.html#configuring-categories"));
        return {};
    }

#if QT_VERSION >= QT_VERSION_CHECK(5, 14, 0)
    auto filterRules = QList(ctx.words.begin() + 1, ctx.words.end()).join('\n');
#else
    auto filterRules = [&]() {
        auto tmpList = ctx.words;
        tmpList.pop_front();
        return tmpList.join('\n');
    }();
#endif

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

}  // namespace chatterino::commands
