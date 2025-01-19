#include "common/QLogging.hpp"

#ifdef NDEBUG
static constexpr QtMsgType logThreshold = QtWarningMsg;
#else
static constexpr QtMsgType logThreshold = QtDebugMsg;
#endif

Q_LOGGING_CATEGORY(chatterinoApp, "chatterino.app", logThreshold);
Q_LOGGING_CATEGORY(chatterinoArgs, "chatterino.args", logThreshold);
Q_LOGGING_CATEGORY(chatterinoBenchmark, "chatterino.benchmark", logThreshold);
Q_LOGGING_CATEGORY(chatterinoBttv, "chatterino.bttv", logThreshold);
Q_LOGGING_CATEGORY(chatterinoCache, "chatterino.cache", logThreshold);
Q_LOGGING_CATEGORY(chatterinoCommands, "chatterino.commands", logThreshold);
Q_LOGGING_CATEGORY(chatterinoCommon, "chatterino.common", logThreshold);
Q_LOGGING_CATEGORY(chatterinoCrashhandler, "chatterino.crashhandler",
                   logThreshold);
Q_LOGGING_CATEGORY(chatterinoEmoji, "chatterino.emoji", logThreshold);
Q_LOGGING_CATEGORY(chatterinoEnv, "chatterino.env", logThreshold);
Q_LOGGING_CATEGORY(chatterinoFfzemotes, "chatterino.ffzemotes", logThreshold);
Q_LOGGING_CATEGORY(chatterinoHelper, "chatterino.helper", logThreshold);
Q_LOGGING_CATEGORY(chatterinoHighlights, "chatterino.highlights", logThreshold);
Q_LOGGING_CATEGORY(chatterinoHotkeys, "chatterino.hotkeys", logThreshold);
Q_LOGGING_CATEGORY(chatterinoHTTP, "chatterino.http", logThreshold);
Q_LOGGING_CATEGORY(chatterinoImage, "chatterino.image", logThreshold);
Q_LOGGING_CATEGORY(chatterinoIrc, "chatterino.irc", logThreshold);
Q_LOGGING_CATEGORY(chatterinoIvr, "chatterino.ivr", logThreshold);
Q_LOGGING_CATEGORY(chatterinoLiveupdates, "chatterino.liveupdates",
                   logThreshold);
Q_LOGGING_CATEGORY(chatterinoLua, "chatterino.lua", logThreshold);
Q_LOGGING_CATEGORY(chatterinoMain, "chatterino.main", logThreshold);
Q_LOGGING_CATEGORY(chatterinoMessage, "chatterino.message", logThreshold);
Q_LOGGING_CATEGORY(chatterinoNativeMessage, "chatterino.nativemessage",
                   logThreshold);
Q_LOGGING_CATEGORY(chatterinoNetwork, "chatterino.network", logThreshold);
Q_LOGGING_CATEGORY(chatterinoNotification, "chatterino.notification",
                   logThreshold);
Q_LOGGING_CATEGORY(chatterinoImageuploader, "chatterino.imageuploader",
                   logThreshold);
Q_LOGGING_CATEGORY(chatterinoPronouns, "chatterino.pronouns", logThreshold);
Q_LOGGING_CATEGORY(chatterinoPubSub, "chatterino.pubsub", logThreshold);
Q_LOGGING_CATEGORY(chatterinoRecentMessages, "chatterino.recentmessages",
                   logThreshold);
Q_LOGGING_CATEGORY(chatterinoSettings, "chatterino.settings", logThreshold);
Q_LOGGING_CATEGORY(chatterinoSeventv, "chatterino.seventv", logThreshold);
Q_LOGGING_CATEGORY(chatterinoSeventvEventAPI, "chatterino.seventv.eventapi",
                   logThreshold);
Q_LOGGING_CATEGORY(chatterinoSound, "chatterino.sound", logThreshold);
Q_LOGGING_CATEGORY(chatterinoStreamerMode, "chatterino.streamermode",
                   logThreshold);
Q_LOGGING_CATEGORY(chatterinoStreamlink, "chatterino.streamlink", logThreshold);
Q_LOGGING_CATEGORY(chatterinoTheme, "chatterino.theme", logThreshold);
Q_LOGGING_CATEGORY(chatterinoTokenizer, "chatterino.tokenizer", logThreshold);
Q_LOGGING_CATEGORY(chatterinoTwitch, "chatterino.twitch", logThreshold);
Q_LOGGING_CATEGORY(chatterinoTwitchEventSub, "chatterino.twitch.eventsub",
                   logThreshold);
Q_LOGGING_CATEGORY(chatterinoTwitchLiveController,
                   "chatterino.twitch.livecontroller", logThreshold);
Q_LOGGING_CATEGORY(chatterinoUpdate, "chatterino.update", logThreshold);
Q_LOGGING_CATEGORY(chatterinoWebsocket, "chatterino.websocket", logThreshold);
Q_LOGGING_CATEGORY(chatterinoWidget, "chatterino.widget", logThreshold);
Q_LOGGING_CATEGORY(chatterinoWindowmanager, "chatterino.windowmanager",
                   logThreshold);
Q_LOGGING_CATEGORY(chatterinoXDG, "chatterino.xdg", logThreshold);
