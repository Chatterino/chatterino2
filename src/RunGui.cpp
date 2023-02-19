#include "RunGui.hpp"

#include "Application.hpp"
#include "common/Args.hpp"
#include "common/Modes.hpp"
#include "common/NetworkManager.hpp"
#include "common/QLogging.hpp"
#include "singletons/Paths.hpp"
#include "singletons/Resources.hpp"
#include "singletons/Settings.hpp"
#include "singletons/Updates.hpp"
#include "util/CombinePath.hpp"
#include "widgets/dialogs/LastRunCrashDialog.hpp"

#include <QApplication>
#include <QFile>
#include <QPalette>
#include <QStyleFactory>
#include <Qt>
#include <QtConcurrent>

#include <csignal>
#include <ranges>

#ifdef USEWINSDK
#    include "util/WindowsHelper.hpp"
#endif

#ifdef C_USE_BREAKPAD
#    include <QBreakpadHandler.h>
#endif

#ifdef Q_OS_MAC
#    include "corefoundation/CFBundle.h"
#endif

namespace chatterino {
namespace {
    void installCustomPalette()
    {
        // borrowed from
        // https://stackoverflow.com/questions/15035767/is-the-qt-5-dark-fusion-theme-available-for-windows
        auto dark = qApp->palette();

        dark.setColor(QPalette::Window, QColor(22, 22, 22));
        dark.setColor(QPalette::WindowText, Qt::white);
        dark.setColor(QPalette::Text, Qt::white);
        dark.setColor(QPalette::Disabled, QPalette::WindowText,
                      QColor(127, 127, 127));
        dark.setColor(QPalette::Base, QColor("#333"));
        dark.setColor(QPalette::AlternateBase, QColor("#444"));
        dark.setColor(QPalette::ToolTipBase, Qt::white);
        dark.setColor(QPalette::ToolTipText, Qt::white);
        dark.setColor(QPalette::Disabled, QPalette::Text,
                      QColor(127, 127, 127));
        dark.setColor(QPalette::Dark, QColor(35, 35, 35));
        dark.setColor(QPalette::Shadow, QColor(20, 20, 20));
        dark.setColor(QPalette::Button, QColor(70, 70, 70));
        dark.setColor(QPalette::ButtonText, Qt::white);
        dark.setColor(QPalette::Disabled, QPalette::ButtonText,
                      QColor(127, 127, 127));
        dark.setColor(QPalette::BrightText, Qt::red);
        dark.setColor(QPalette::Link, QColor(42, 130, 218));
        dark.setColor(QPalette::Highlight, QColor(42, 130, 218));
        dark.setColor(QPalette::Disabled, QPalette::Highlight,
                      QColor(80, 80, 80));
        dark.setColor(QPalette::HighlightedText, Qt::white);
        dark.setColor(QPalette::Disabled, QPalette::HighlightedText,
                      QColor(127, 127, 127));

        qApp->setPalette(dark);
    }

    void initQt()
    {
        // set up the QApplication flags
        QApplication::setAttribute(Qt::AA_Use96Dpi, true);
#ifdef Q_OS_WIN32
        QApplication::setAttribute(Qt::AA_DisableHighDpiScaling, true);
#endif

        QApplication::setStyle(QStyleFactory::create("Fusion"));

        QApplication::setWindowIcon(QIcon(":/icon.ico"));

        installCustomPalette();
    }

    void showLastCrashDialog()
    {
        //#ifndef C_DISABLE_CRASH_DIALOG
        //        LastRunCrashDialog dialog;

        //        switch (dialog.exec())
        //        {
        //            case QDialog::Accepted:
        //            {
        //            };
        //            break;
        //            default:
        //            {
        //                _exit(0);
        //            }
        //        }
        //#endif
    }

    void createRunningFile(const QString &path)
    {
        QFile runningFile(path);

        runningFile.open(QIODevice::WriteOnly | QIODevice::Truncate);
        runningFile.flush();
        runningFile.close();
    }

    void removeRunningFile(const QString &path)
    {
        QFile::remove(path);
    }

    std::chrono::steady_clock::time_point signalsInitTime;
    bool restartOnSignal = false;

    [[noreturn]] void handleSignal(int signum)
    {
        using namespace std::chrono_literals;

        if (restartOnSignal &&
            std::chrono::steady_clock::now() - signalsInitTime > 30s)
        {
            QProcess proc;

#ifdef Q_OS_MAC
            // On macOS, programs are bundled into ".app" Application bundles,
            // when restarting Chatterino that bundle should be opened with the "open"
            // terminal command instead of directly starting the underlying executable,
            // as those are 2 different things for the OS and i.e. do not use
            // the same dock icon (resulting in a second Chatterino icon on restarting)
            CFURLRef appUrlRef = CFBundleCopyBundleURL(CFBundleGetMainBundle());
            CFStringRef macPath =
                CFURLCopyFileSystemPath(appUrlRef, kCFURLPOSIXPathStyle);
            const char *pathPtr =
                CFStringGetCStringPtr(macPath, CFStringGetSystemEncoding());

            proc.setProgram("open");
            proc.setArguments({pathPtr, "-n", "--args", "--crash-recovery"});

            CFRelease(appUrlRef);
            CFRelease(macPath);
#else
            proc.setProgram(QApplication::applicationFilePath());
            proc.setArguments({"--crash-recovery"});
#endif

            proc.startDetached();
        }

        _exit(signum);
    }

    // We want to restart Chatterino when it crashes and the setting is set to
    // true.
    void initSignalHandler()
    {
#if defined(NDEBUG) && !defined(CHATTERINO_WITH_CRASHPAD)
        signalsInitTime = std::chrono::steady_clock::now();

        signal(SIGSEGV, handleSignal);
#endif
    }

    // We delete cache files that haven't been modified in 14 days. This strategy may be
    // improved in the future.
    void clearCache(const QDir &dir)
    {
        int deletedCount = 0;
        for (auto &&info : dir.entryInfoList(QDir::Files))
        {
            if (info.lastModified().addDays(14) < QDateTime::currentDateTime())
            {
                bool res = QFile(info.absoluteFilePath()).remove();
                if (res)
                    ++deletedCount;
            }
        }
        qCDebug(chatterinoCache) << "Deleted" << deletedCount << "files";
    }

    // We delete all but the five most recent crashdumps. This strategy may be
    // improved in the future.
    void clearCrashes(QDir dir)
    {
        dir.setNameFilters({"*.dmp"});

        size_t deletedCount = 0;
        for (auto &&info :
             dir.entryInfoList(QDir::Files, QDir::Time) | std::views::drop(5))
        {
            if (QFile(info.absoluteFilePath()).remove())
            {
                deletedCount++;
            }
        }
        qCDebug(chatterinoApp) << "Deleted" << deletedCount << "crashdumps";
    }
}  // namespace

void runGui(QApplication &a, Paths &paths, Settings &settings)
{
    initQt();
    initResources();
    initSignalHandler();

    settings.restartOnCrash.connect([](const bool &value) {
        restartOnSignal = value;
    });

    auto thread = std::thread([dir = paths.miscDirectory] {
        {
            auto path = combinePath(dir, "Update.exe");
            if (QFile::exists(path))
            {
                QFile::remove(path);
            }
        }
        {
            auto path = combinePath(dir, "update.zip");
            if (QFile::exists(path))
            {
                QFile::remove(path);
            }
        }
    });

    // Clear the cache 1 minute after start.
    QTimer::singleShot(60 * 1000, [cachePath = paths.cacheDirectory(),
                                   crashDirectory = paths.crashdumpDirectory] {
        QtConcurrent::run([cachePath]() {
            clearCache(cachePath);
        });

        auto crashReportsDir = QDir(crashDirectory);
        // check if the /reports directory exists
        if (crashReportsDir.cd("reports"))
        {
            QtConcurrent::run([crashReportsDir]() {
                clearCrashes(crashReportsDir);
            });
        }
    });

    chatterino::NetworkManager::init();
    chatterino::Updates::instance().checkForUpdates();

#ifdef C_USE_BREAKPAD
    QBreakpadInstance.setDumpPath(getPaths()->settingsFolderPath + "/Crashes");
#endif

    // Running file
    auto runningPath =
        paths.miscDirectory + "/running_" + paths.applicationFilePathHash;

    if (QFile::exists(runningPath))
    {
        showLastCrashDialog();
    }
    else
    {
        createRunningFile(runningPath);
    }

    Application app(settings, paths);
    app.initialize(settings, paths);
    app.run(a);
    app.save();

    removeRunningFile(runningPath);

    if (!getArgs().dontSaveSettings)
    {
        pajlada::Settings::SettingManager::gSave();
    }

    chatterino::NetworkManager::deinit();

#ifdef USEWINSDK
    // flushing windows clipboard to keep copied messages
    flushClipboard();
#endif

    _exit(0);
}
}  // namespace chatterino
