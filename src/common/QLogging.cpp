#include "common/QLogging.hpp"

#ifdef DEBUG_OFF
static constexpr QtMsgType logThreshold = QtWarningMsg;
#else
static constexpr QtMsgType logThreshold = QtDebugMsg;
#endif

Q_LOGGING_CATEGORY(chatterinoApp, "chatterino.app", logThreshold);
Q_LOGGING_CATEGORY(chatterinoArgs, "chatterino.args", logThreshold);
Q_LOGGING_CATEGORY(chatterinoBenchmark, "chatterino.benchmark", logThreshold);
Q_LOGGING_CATEGORY(chatterinoBttv, "chatterino.bttv", logThreshold);
Q_LOGGING_CATEGORY(chatterinoCommon, "chatterino.cache", logThreshold);
Q_LOGGING_CATEGORY(chatterinoCache, "chatterino.common", logThreshold);
Q_LOGGING_CATEGORY(chatterinoEmoji, "chatterino.emoji", logThreshold);
Q_LOGGING_CATEGORY(chatterinoFfzemotes, "chatterino.ffzemotes", logThreshold);
Q_LOGGING_CATEGORY(chatterinoHelper, "chatterino.helper", logThreshold);
Q_LOGGING_CATEGORY(chatterinoImage, "chatterino.image", logThreshold);
Q_LOGGING_CATEGORY(chatterinoIrc, "chatterino.irc", logThreshold);
Q_LOGGING_CATEGORY(chatterinoIvr, "chatterino.ivr", logThreshold);
Q_LOGGING_CATEGORY(chatterinoMain, "chatterino.main", logThreshold);
Q_LOGGING_CATEGORY(chatterinoMessage, "chatterino.message", logThreshold);
Q_LOGGING_CATEGORY(chatterinoNativeMessage, "chatterino.nativemessage",
                   logThreshold);
Q_LOGGING_CATEGORY(chatterinoNotification, "chatterino.notification",
                   logThreshold);
Q_LOGGING_CATEGORY(chatterinoNuulsuploader, "chatterino.nuulsuploader",
                   logThreshold);
Q_LOGGING_CATEGORY(chatterinoPubsub, "chatterino.pubsub", logThreshold);
Q_LOGGING_CATEGORY(chatterinoStreamlink, "chatterino.streamlink", logThreshold);
Q_LOGGING_CATEGORY(chatterinoTokenizer, "chatterino.tokenizer", logThreshold);
Q_LOGGING_CATEGORY(chatterinoTwitch, "chatterino.twitch", logThreshold);
Q_LOGGING_CATEGORY(chatterinoUpdate, "chatterino.update", logThreshold);
Q_LOGGING_CATEGORY(chatterinoWebsocket, "chatterino.websocket", logThreshold);
Q_LOGGING_CATEGORY(chatterinoWidget, "chatterino.widget", logThreshold);
Q_LOGGING_CATEGORY(chatterinoWindowmanager, "chatterino.windowmanager",
                   logThreshold);
