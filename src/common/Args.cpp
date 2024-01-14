#include "common/Args.hpp"

#include "common/QLogging.hpp"
#include "debug/AssertInGuiThread.hpp"
#include "singletons/Paths.hpp"
#include "singletons/WindowManager.hpp"
#include "util/AttachToConsole.hpp"
#include "util/CombinePath.hpp"
#include "widgets/Window.hpp"

#include <QApplication>
#include <QCommandLineParser>
#include <QDebug>
#include <QRegularExpression>
#include <QStringList>
#include <QUuid>

namespace {

template <class... Args>
QCommandLineOption hiddenOption(Args... args)
{
    QCommandLineOption opt(args...);
    opt.setFlags(QCommandLineOption::HiddenFromHelp);
    return opt;
}

QStringList extractCommandLine(
    const QCommandLineParser &parser,
    std::initializer_list<QCommandLineOption> options)
{
    QStringList args;
    for (const auto &option : options)
    {
        if (parser.isSet(option))
        {
            auto optionName = option.names().first();
            if (optionName.length() == 1)
            {
                optionName.prepend(u'-');
            }
            else
            {
                optionName.prepend("--");
            }

            auto values = parser.values(option);
            if (values.empty())
            {
                args += optionName;
            }
            else
            {
                for (const auto &value : values)
                {
                    args += optionName;
                    args += value;
                }
            }
        }
    }
    return args;
}

}  // namespace

namespace chatterino {

Args::Args(const QApplication &app)
{
    QCommandLineParser parser;
    parser.setApplicationDescription("Chatterino 2 Client for Twitch Chat");
    parser.addHelpOption();

    // Used internally by app to restart after unexpected crashes
    auto crashRecoveryOption = hiddenOption("crash-recovery");
    auto exceptionCodeOption = hiddenOption("cr-exception-code", "", "code");
    auto exceptionMessageOption =
        hiddenOption("cr-exception-message", "", "message");

    // Added to ignore the parent-window option passed during native messaging
    auto parentWindowOption = hiddenOption("parent-window");
    auto parentWindowIdOption =
        hiddenOption("x-attach-split-to-window", "", "window-id");

    // Verbose
    auto verboseOption = QCommandLineOption(
        QStringList{"v", "verbose"}, "Attaches to the Console on windows, "
                                     "allowing you to see debug output.");
    // Safe mode
    QCommandLineOption safeModeOption(
        "safe-mode", "Starts Chatterino without loading Plugins and always "
                     "show the settings button.");

    // Channel layout
    auto channelLayout = QCommandLineOption(
        {"c", "channels"},
        "Joins only supplied channels on startup. Use letters with colons to "
        "specify platform. Only Twitch channels are supported at the moment.\n"
        "If platform isn't specified, default is Twitch.",
        "t:channel1;t:channel2;...");

    parser.addOptions({
        {{"V", "version"}, "Displays version information."},
        crashRecoveryOption,
        exceptionCodeOption,
        exceptionMessageOption,
        parentWindowOption,
        parentWindowIdOption,
        verboseOption,
        safeModeOption,
        channelLayout,
    });

    if (!parser.parse(app.arguments()))
    {
        qCWarning(chatterinoArgs)
            << "Unhandled options:" << parser.unknownOptionNames();
    }

    if (parser.isSet("help"))
    {
        attachToConsole();
        qInfo().noquote() << parser.helpText();
        ::exit(EXIT_SUCCESS);
    }

    const QStringList args = parser.positionalArguments();
    this->shouldRunBrowserExtensionHost =
        (args.size() > 0 && (args[0].startsWith("chrome-extension://") ||
                             args[0].endsWith(".json")));

    if (parser.isSet(channelLayout))
    {
        this->applyCustomChannelLayout(parser.value(channelLayout));
    }

    this->verbose = parser.isSet(verboseOption);

    this->printVersion = parser.isSet("V");

    this->crashRecovery = parser.isSet(crashRecoveryOption);
    if (parser.isSet(exceptionCodeOption))
    {
        this->exceptionCode =
            static_cast<uint32_t>(parser.value(exceptionCodeOption).toULong());
    }
    if (parser.isSet(exceptionMessageOption))
    {
        this->exceptionMessage = parser.value(exceptionMessageOption);
    }

    if (parser.isSet(parentWindowIdOption))
    {
        this->isFramelessEmbed = true;
        this->dontSaveSettings = true;
        this->dontLoadMainWindow = true;

        this->parentWindowId = parser.value(parentWindowIdOption).toULongLong();
    }
    if (parser.isSet(safeModeOption))
    {
        this->safeMode = true;
    }

    this->currentArguments_ = extractCommandLine(parser, {
                                                             verboseOption,
                                                             safeModeOption,
                                                             channelLayout,
                                                         });
}

QStringList Args::currentArguments() const
{
    return this->currentArguments_;
}

void Args::applyCustomChannelLayout(const QString &argValue)
{
    WindowLayout layout;
    WindowDescriptor window;

    /*
     * There is only one window that is loaded from the --channels
     * argument so that is what we use as the main window.
     */
    window.type_ = WindowType::Main;

    // Load main window layout from config file so we can use the same geometry
    const QRect configMainLayout = [] {
        const QString windowLayoutFile =
            combinePath(getPaths()->settingsDirectory,
                        WindowManager::WINDOW_LAYOUT_FILENAME);

        const WindowLayout configLayout =
            WindowLayout::loadFromFile(windowLayoutFile);

        for (const WindowDescriptor &window : configLayout.windows_)
        {
            if (window.type_ != WindowType::Main)
            {
                continue;
            }

            return window.geometry_;
        }

        return QRect(-1, -1, -1, -1);
    }();

    window.geometry_ = configMainLayout;

    QStringList channelArgList = argValue.split(";");
    for (const QString &channelArg : channelArgList)
    {
        if (channelArg.isEmpty())
        {
            continue;
        }

        // Twitch is default platform
        QString platform = "t";
        QString channelName = channelArg;

        const QRegularExpression regExp("(.):(.*)");
        if (auto match = regExp.match(channelArg); match.hasMatch())
        {
            platform = match.captured(1);
            channelName = match.captured(2);
        }

        // Twitch (default)
        if (platform == "t")
        {
            TabDescriptor tab;

            // Set first tab as selected
            tab.selected_ = window.tabs_.empty();
            tab.rootNode_ = SplitNodeDescriptor{{"twitch", channelName}};

            window.tabs_.emplace_back(std::move(tab));
        }
    }

    // Only respect --channels if we could actually parse any channels
    if (!window.tabs_.empty())
    {
        this->dontSaveSettings = true;

        layout.windows_.emplace_back(std::move(window));
        this->customChannelLayout = std::move(layout);
    }
}

}  // namespace chatterino
