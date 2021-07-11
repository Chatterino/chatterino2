#include "Args.hpp"

#include "common/QLogging.hpp"
#include "singletons/Paths.hpp"
#include "singletons/WindowManager.hpp"
#include "util/AttachToConsole.hpp"
#include "util/CombinePath.hpp"
#include "widgets/Window.hpp"

#include <QApplication>
#include <QCommandLineParser>
#include <QDebug>
#include <QStringList>
#include <QUuid>

namespace chatterino {

Args::Args(const QApplication &app)
{
    QCommandLineParser parser;
    parser.setApplicationDescription("Chatterino 2 Client for Twitch Chat");
    parser.addHelpOption();

    // Used internally by app to restart after unexpected crashes
    QCommandLineOption crashRecoveryOption("crash-recovery");
    crashRecoveryOption.setFlags(QCommandLineOption::HiddenFromHelp);

    // Added to ignore the parent-window option passed during native messaging
    QCommandLineOption parentWindowOption("parent-window");
    parentWindowOption.setFlags(QCommandLineOption::HiddenFromHelp);
    QCommandLineOption parentWindowIdOption("x-attach-split-to-window", "",
                                            "window-id");
    parentWindowIdOption.setFlags(QCommandLineOption::HiddenFromHelp);

    // Verbose
    QCommandLineOption verboseOption({{"v", "verbose"},
                                      "Attaches to the Console on windows, "
                                      "allowing you to see debug output."});
    crashRecoveryOption.setFlags(QCommandLineOption::HiddenFromHelp);

    parser.addOptions({
        {{"V", "version"}, "Displays version information."},
        crashRecoveryOption,
        parentWindowOption,
        parentWindowIdOption,
        verboseOption,
    });
    parser.addOption(QCommandLineOption(
        {"c", "channels"},
        "Joins only supplied channels on startup. Use letters with colons to "
        "specify platform. Only twitch channels are supported at the moment.\n"
        "If platform isn't specified, default is Twitch.",
        "t:channel1;t:channel2;..."));

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

    if (parser.isSet("c"))
    {
        this->applyCustomChannelLayout(parser.value("c"));
    }

    this->verbose = parser.isSet(verboseOption);

    this->printVersion = parser.isSet("V");
    this->crashRecovery = parser.isSet("crash-recovery");

    if (parser.isSet(parentWindowIdOption))
    {
        this->isFramelessEmbed = true;
        this->dontSaveSettings = true;
        this->dontLoadMainWindow = true;

        this->parentWindowId = parser.value(parentWindowIdOption).toULongLong();
    }
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
                continue;

            return window.geometry_;
        }

        return QRect(-1, -1, -1, -1);
    }();

    window.geometry_ = configMainLayout;

    QStringList channelArgList = argValue.split(";");
    for (const QString &channelArg : channelArgList)
    {
        if (channelArg.isEmpty())
            continue;

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

static Args *instance = nullptr;

void initArgs(const QApplication &app)
{
    instance = new Args(app);
}

const Args &getArgs()
{
    assert(instance);

    return *instance;
}

}  // namespace chatterino
