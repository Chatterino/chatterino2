#include "RunGui.hpp"

#include "Application.hpp"
#include "common/Args.hpp"
#include "common/Modes.hpp"
#include "common/network/NetworkManager.hpp"
#include "common/QLogging.hpp"
#include "singletons/CrashHandler.hpp"
#include "singletons/Paths.hpp"
#include "singletons/Resources.hpp"
#include "singletons/Settings.hpp"
#include "singletons/Updates.hpp"
#include "util/CombinePath.hpp"
#include "util/UnixSignalHandler.hpp"
#include "widgets/dialogs/LastRunCrashDialog.hpp"

#include <QApplication>
#include <QFile>
#include <QPalette>
#include <QStyleFactory>
#include <Qt>
#include <QtConcurrent>

#include <csignal>
#include <tuple>

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
        auto dark = QApplication::palette();

        dark.setColor(QPalette::Window, QColor(22, 22, 22));
        dark.setColor(QPalette::WindowText, Qt::white);
        dark.setColor(QPalette::Text, Qt::white);
        dark.setColor(QPalette::Base, QColor("#333"));
        dark.setColor(QPalette::AlternateBase, QColor("#444"));
        dark.setColor(QPalette::ToolTipBase, Qt::white);
        dark.setColor(QPalette::ToolTipText, Qt::black);
        dark.setColor(QPalette::Dark, QColor(35, 35, 35));
        dark.setColor(QPalette::Shadow, QColor(20, 20, 20));
        dark.setColor(QPalette::Button, QColor(70, 70, 70));
        dark.setColor(QPalette::ButtonText, Qt::white);
        dark.setColor(QPalette::BrightText, Qt::red);
        dark.setColor(QPalette::Link, QColor(42, 130, 218));
        dark.setColor(QPalette::Highlight, QColor(42, 130, 218));
        dark.setColor(QPalette::HighlightedText, Qt::white);
        dark.setColor(QPalette::PlaceholderText, QColor(127, 127, 127));

        dark.setColor(QPalette::Disabled, QPalette::Highlight,
                      QColor(80, 80, 80));
        dark.setColor(QPalette::Disabled, QPalette::HighlightedText,
                      QColor(127, 127, 127));
        dark.setColor(QPalette::Disabled, QPalette::ButtonText,
                      QColor(127, 127, 127));
        dark.setColor(QPalette::Disabled, QPalette::Text,
                      QColor(127, 127, 127));
        dark.setColor(QPalette::Disabled, QPalette::WindowText,
                      QColor(127, 127, 127));

        QApplication::setPalette(dark);
    }

    void initQt()
    {
        // set up the QApplication flags
        QApplication::setAttribute(Qt::AA_Use96Dpi, true);

#ifdef Q_OS_WIN32
        // Avoid promoting child widgets to child windows
        // This causes bugs with frameless windows as not all child events
        // get sent to the parent - effectively making the window immovable.
        QApplication::setAttribute(Qt::AA_DontCreateNativeWidgetSiblings);
#endif

        QApplication::setStyle(QStyleFactory::create("Fusion"));

#ifndef Q_OS_MAC
        QApplication::setWindowIcon(QIcon(":/icon.ico"));
#endif

#ifdef Q_OS_MAC
        // On the Mac/Cocoa platform this attribute is enabled by default
        // We override it to ensure shortcuts show in context menus on that platform
        QApplication::setAttribute(Qt::AA_DontShowShortcutsInContextMenus,
                                   false);
#endif

        installCustomPalette();
    }

    void showLastCrashDialog(const Args &args, const Paths &paths)
    {
        auto *dialog = new LastRunCrashDialog(args, paths);
        // Use exec() over open() to block the app from being loaded
        // and to be able to set the safe mode.
        dialog->exec();
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

    [[noreturn]] void handleSignal(int signum)
    {
        using namespace std::chrono_literals;

        if (std::chrono::steady_clock::now() - signalsInitTime > 30s &&
            getApp()->getCrashHandler()->shouldRecover())
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

#if defined(Q_OS_UNIX)
        auto *sigintHandler = new UnixSignalHandler(SIGINT);
        QObject::connect(sigintHandler, &UnixSignalHandler::signalFired, [] {
            qCInfo(chatterinoApp)
                << "Received SIGINT, request application quit";
            QApplication::quit();
        });
        auto *sigtermHandler = new UnixSignalHandler(SIGTERM);
        QObject::connect(sigtermHandler, &UnixSignalHandler::signalFired, [] {
            qCInfo(chatterinoApp)
                << "Received SIGTERM, request application quit";
            QApplication::quit();
        });
#endif
    }

    // We delete cache files that haven't been modified in 14 days. This strategy may be
    // improved in the future.
    void clearCache(const QDir &dir)
    {
        size_t deletedCount = 0;
        for (const auto &info : dir.entryInfoList(QDir::Files))
        {
            if (info.lastModified().addDays(14) < QDateTime::currentDateTime())
            {
                bool res = QFile(info.absoluteFilePath()).remove();
                if (res)
                {
                    ++deletedCount;
                }
            }
        }
        qCDebug(chatterinoCache)
            << "Deleted" << deletedCount << "files in" << dir.path();
    }

    // We delete all but the five most recent crashdumps. This strategy may be
    // improved in the future.
    void clearCrashes(QDir dir)
    {
        // crashpad crashdumps are stored inside the Crashes/report directory
        if (!dir.cd("reports"))
        {
            // no reports directory exists = no files to delete
            return;
        }

        dir.setNameFilters({"*.dmp"});

        size_t deletedCount = 0;
        // TODO: use std::views::drop once supported by all compilers
        size_t filesToSkip = 5;
        for (auto &&info : dir.entryInfoList(QDir::Files, QDir::Time))
        {
            if (filesToSkip > 0)
            {
                filesToSkip--;
                continue;
            }

            if (QFile(info.absoluteFilePath()).remove())
            {
                deletedCount++;
            }
        }
        qCDebug(chatterinoApp) << "Deleted" << deletedCount << "crashdumps";
    }
}  // namespace

void runGui(QApplication &a, const Paths &paths, Settings &settings,
            const Args &args, Updates &updates)
{
    initQt();
    initResources();
    initSignalHandler();

#ifdef Q_OS_WIN
    if (args.crashRecovery)
    {
        showLastCrashDialog(args, paths);
    }
#endif

    updates.deleteOldFiles();

    // Clear the cache 1 minute after start.
    QTimer::singleShot(60 * 1000, [cachePath = paths.cacheDirectory(),
                                   crashDirectory = paths.crashdumpDirectory,
                                   avatarPath = paths.twitchProfileAvatars] {
        std::ignore = QtConcurrent::run([cachePath] {
            clearCache(cachePath);
        });
        std::ignore = QtConcurrent::run([avatarPath] {
            clearCache(avatarPath);
        });
        std::ignore = QtConcurrent::run([crashDirectory] {
            clearCrashes(crashDirectory);
        });
    });

    chatterino::NetworkManager::init();
    updates.checkForUpdates();

    Application app(settings, paths, args, updates);
    app.initialize(settings, paths);
    app.run();
    app.save();

    settings.requestSave();

    chatterino::NetworkManager::deinit();

#ifdef USEWINSDK
    // flushing windows clipboard to keep copied messages
    flushClipboard();
#endif
}

}  // namespace chatterino
