#include "widgets/dialogs/LastRunCrashDialog.hpp"

#include "common/Args.hpp"
#include "common/Literals.hpp"
#include "common/Modes.hpp"
#include "singletons/Paths.hpp"
#include "util/LayoutCreator.hpp"

#include <QDesktopServices>
#include <QDialogButtonBox>
#include <QDir>
#include <QLabel>
#include <QMessageBox>
#include <QProcess>
#include <QPushButton>
#include <QRandomGenerator>
#include <QStringBuilder>
#include <QVBoxLayout>

namespace {

using namespace chatterino::literals;

const std::vector<QString> MESSAGES = {
    u"Oops..."_s,
    u"NotLikeThis"_s,
    u"NOOOOOO"_s,
    u"I'm sorry"_s,
    u"We're sorry"_s,
    u"My bad"_s,
    u"FailFish"_s,
    u"O_o"_s,
    uR"("%/§*'"$)%=})"_s,
    u"Sorry :("_s,
    u"I blame cosmic rays"_s,
    u"I blame TMI"_s,
    u"I blame Helix"_s,
    // "Wtf is Utf16?" (but with swapped endian)
    u"圀琀昀\u2000椀猀\u2000唀琀昀㄀㘀㼀"_s,
    u"Oopsie woopsie"_s,
};

QString randomMessage()
{
#if QT_VERSION < QT_VERSION_CHECK(6, 0, 0)
    using Ty = quint32;
#else
    using Ty = quint64;
#endif
    return MESSAGES[static_cast<size_t>(
        QRandomGenerator::global()->bounded(static_cast<Ty>(MESSAGES.size())))];
}

}  // namespace

namespace chatterino {

using namespace literals;

LastRunCrashDialog::LastRunCrashDialog()
{
    this->setWindowFlag(Qt::WindowContextHelpButtonHint, false);
    this->setWindowTitle(u"Chatterino - " % randomMessage());

    auto layout =
        LayoutCreator<LastRunCrashDialog>(this).setLayoutType<QVBoxLayout>();

    QString text =
        u"Chatterino unexpectedly crashed and restarted. "_s
        "<i>You can disable automatic restarts in the settings.</i><br><br>";

#ifdef CHATTERINO_WITH_CRASHPAD
    auto reportsDir =
        QDir(getPaths()->crashdumpDirectory).filePath(u"reports"_s);
    text += u"A <b>crash report</b> has been saved to "
            "<a href=\"file:///" %
            reportsDir % u"\">" % reportsDir % u"</a>.<br>";

    if (getArgs().exceptionCode)
    {
        text += u"The last run crashed with code <code>0x" %
                QString::number(*getArgs().exceptionCode, 16) % u"</code>";

        if (getArgs().exceptionMessage)
        {
            text += u" (" % *getArgs().exceptionMessage % u")";
        }

        text += u".<br>"_s;
    }

    if (getArgs().extraMemory && *getArgs().extraMemory > 0)
    {
        text += QLocale::system().formattedDataSize(
                    static_cast<qint64>(*getArgs().extraMemory)) %
                " have been included in the report.<br>";
    }
    else
    {
        text +=
            u"No extra memory snapshot was saved with the crash report.<br>"_s;
    }

    text +=
        "Crash reports are <b>only stored locally</b> and never uploaded.<br>"
        u"<br>Please report the crash so it can be prevented in the future."_s;

    if (Modes::instance().isNightly)
    {
        text += u" Make sure you're using the latest nightly version!"_s;
    }

    text +=
        u"<br>For more information, <a href=\"https://wiki.chatterino.com/Crash%20Analysis/\">consult the wiki</a>."_s;
#endif

    auto label = layout.emplace<QLabel>(text);
    label->setTextInteractionFlags(Qt::TextBrowserInteraction);
    label->setOpenExternalLinks(true);
    label->setWordWrap(true);

    layout->addSpacing(16);

    auto buttons = layout.emplace<QDialogButtonBox>();

    auto *okButton = buttons->addButton(u"Ok"_s, QDialogButtonBox::AcceptRole);
    QObject::connect(okButton, &QPushButton::clicked, [this] {
        this->accept();
    });

    if (!getArgs().safeMode)
    {
        auto *safeModeButton = buttons->addButton(u"Restart in safe mode"_s,
                                                  QDialogButtonBox::NoRole);
        safeModeButton->setToolTipDuration(0);
        safeModeButton->setToolTip(
            u"In safe mode, the settings button is always shown"_s
#ifdef CHATTERINO_HAVE_PLUGINS
            " and plugins are disabled"
#endif
            ".\nSame as starting with --safe-mode.");

        QObject::connect(safeModeButton, &QPushButton::clicked, [this] {
            auto args = getArgs().currentArguments();
            args += u"--safe-mode"_s;
            if (QProcess::startDetached(qApp->applicationFilePath(), args))
            {
                // We have to do this as we show this dialog with exec()
                _Exit(0);
                return;
            }

            QMessageBox::critical(
                this, u"Error"_s,
                u"Failed to start the app with --safe-mode. Please restart the app manually."_s,
                QMessageBox::Close);
            QApplication::exit(0);
        });
    }
}

}  // namespace chatterino
