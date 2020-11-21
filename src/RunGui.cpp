#include "RunGui.hpp"

#include <QApplication>
#include <QFile>
#include <QPalette>
#include <QStyleFactory>
#include <Qt>
#include <QtConcurrent>
#include <csignal>

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

#ifdef USEWINSDK
#    include "util/WindowsHelper.hpp"
#endif

#ifdef C_USE_BREAKPAD
#    include <QBreakpadHandler.h>
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
            proc.setProgram(QApplication::applicationFilePath());
            proc.setArguments({"--crash-recovery"});
            proc.startDetached();
        }

        _exit(signum);
    }

    // We want to restart chatterino when it crashes and the setting is set to
    // true.
    void initSignalHandler()
    {
#ifndef C_DEBUG
        signalsInitTime = std::chrono::steady_clock::now();

        signal(SIGSEGV, handleSignal);
#endif
    }

    // We delete cache files that haven't been modified in 14 days. This strategy may be
    // improved in the future.
    void clearCache(const QDir &dir)
    {
        qCDebug(chatterinoCache) << "[Cache] cleared cache";

        QStringList toBeRemoved;

        for (auto &&info : dir.entryInfoList(QDir::Files))
        {
            if (info.lastModified().addDays(14) < QDateTime::currentDateTime())
            {
                toBeRemoved << info.absoluteFilePath();
            }
        }
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
    QTimer::singleShot(60 * 1000, [cachePath = paths.cacheDirectory()] {
        QtConcurrent::run([cachePath]() {
            clearCache(cachePath);
        });
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
