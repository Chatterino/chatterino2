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

const std::initializer_list<QString> MESSAGES = {
    u"Oops..."_s,        u"NotLikeThis"_s,
    u"NOOOOOO"_s,        u"I'm sorry"_s,
    u"We're sorry"_s,    u"My bad"_s,
    u"FailFish"_s,       u"O_o"_s,
    u"Sorry :("_s,       u"I blame cosmic rays"_s,
    u"I blame TMI"_s,    u"I blame Helix"_s,
    u"Oopsie woopsie"_s,
};

QString randomMessage()
{
    return *(MESSAGES.begin() +
             (QRandomGenerator::global()->generate64() % MESSAGES.size()));
}

}  // namespace

namespace chatterino {

using namespace literals;

LastRunCrashDialog::LastRunCrashDialog(const Args &args, const Paths &paths)
{
    this->setWindowFlag(Qt::WindowContextHelpButtonHint, false);
    this->setWindowTitle(u"Chatterino - " % randomMessage());

    auto layout =
        LayoutCreator<LastRunCrashDialog>(this).setLayoutType<QVBoxLayout>();

    QString text =
        u"Chatterino unexpectedly crashed and restarted. "_s
        "<i>You can disable automatic restarts in the settings.</i><br><br>";

#ifdef CHATTERINO_WITH_CRASHPAD
    auto reportsDir = QDir(paths.crashdumpDirectory).filePath(u"reports"_s);
    text += u"A <b>crash report</b> has been saved to "
            "<a href=\"file:///" %
            reportsDir % u"\">" % reportsDir % u"</a>.<br>";

    if (args.exceptionCode)
    {
        text += u"The last run crashed with code <code>0x" %
                QString::number(*args.exceptionCode, 16) % u"</code>";

        if (args.exceptionMessage)
        {
            text += u" (" % *args.exceptionMessage % u")";
        }

        text += u".<br>"_s;
    }

    text +=
        "Crash reports are <b>only stored locally</b> and never uploaded.<br>"
        "<br>Please <a "
        "href=\"https://github.com/Chatterino/chatterino2/issues/new\">report "
        "the crash</a> "
        u"so it can be prevented in the future."_s;

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
}

}  // namespace chatterino
