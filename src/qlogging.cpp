#include "qlogging.hpp"

#ifdef DEBUG_OFF
Q_LOGGING_CATEGORY(chatterinoApp, "chatterino.app", QtWarningMsg);
Q_LOGGING_CATEGORY(chatterinoArgs, "chatterino.args", QtWarningMsg);
Q_LOGGING_CATEGORY(chatterinoBenchmark, "chatterino.benchmark", QtWarningMsg);
Q_LOGGING_CATEGORY(chatterinoBttv, "chatterino.bttv", QtWarningMsg);
Q_LOGGING_CATEGORY(chatterinoCommon, "chatterino.cache", QtWarningMsg);
Q_LOGGING_CATEGORY(chatterinoCache, "chatterino.common", QtWarningMsg);
Q_LOGGING_CATEGORY(chatterinoEmoji, "chatterino.emoji", QtWarningMsg);
Q_LOGGING_CATEGORY(chatterinoFfzemotes, "chatterino.ffzemotes", QtWarningMsg);
Q_LOGGING_CATEGORY(chatterinoHelper, "chatterino.helper", QtWarningMsg);
Q_LOGGING_CATEGORY(chatterinoImage, "chatterino.image", QtWarningMsg);
Q_LOGGING_CATEGORY(chatterinoIrc, "chatterino.irc", QtWarningMsg);
Q_LOGGING_CATEGORY(chatterinoIvr, "chatterino.ivr", QtWarningMsg);
Q_LOGGING_CATEGORY(chatterinoMain, "chatterino.main", QtWarningMsg);
Q_LOGGING_CATEGORY(chatterinoMessage, "chatterino.message", QtWarningMsg);
Q_LOGGING_CATEGORY(chatterinoNativeMessage, "chatterino.nativemessage",
                   QtWarningMsg);
Q_LOGGING_CATEGORY(chatterinoNotification, "chatterino.notification",
                   QtWarningMsg);
Q_LOGGING_CATEGORY(chatterinoNuulsuploader, "chatterino.nuulsuploader",
                   QtWarningMsg);
Q_LOGGING_CATEGORY(chatterinoPubsub, "chatterino.pubsub", QtWarningMsg);
Q_LOGGING_CATEGORY(chatterinoStreamlink, "chatterino.streamlink", QtWarningMsg);
Q_LOGGING_CATEGORY(chatterinoTokenizer, "chatterino.tokenizer", QtWarningMsg);
Q_LOGGING_CATEGORY(chatterinoTwitch, "chatterino.twitch", QtWarningMsg);
Q_LOGGING_CATEGORY(chatterinoUpdate, "chatterino.update", QtWarningMsg);
Q_LOGGING_CATEGORY(chatterinoWebsocket, "chatterino.websocket", QtWarningMsg);
Q_LOGGING_CATEGORY(chatterinoWidget, "chatterino.widget", QtWarningMsg);
Q_LOGGING_CATEGORY(chatterinoWindowmanager, "chatterino.windowmanager",
                   QtWarningMsg);
#else
Q_LOGGING_CATEGORY(chatterinoApp, "chatterino.app");
Q_LOGGING_CATEGORY(chatterinoArgs, "chatterino.args");
Q_LOGGING_CATEGORY(chatterinoBenchmark, "chatterino.benchmark");
Q_LOGGING_CATEGORY(chatterinoBttv, "chatterino.bttv");
Q_LOGGING_CATEGORY(chatterinoCommon, "chatterino.cache");
Q_LOGGING_CATEGORY(chatterinoCache, "chatterino.common");
Q_LOGGING_CATEGORY(chatterinoEmoji, "chatterino.emoji");
Q_LOGGING_CATEGORY(chatterinoFfzemotes, "chatterino.ffzemotes");
Q_LOGGING_CATEGORY(chatterinoHelper, "chatterino.helper");
Q_LOGGING_CATEGORY(chatterinoImage, "chatterino.image");
Q_LOGGING_CATEGORY(chatterinoIrc, "chatterino.irc");
Q_LOGGING_CATEGORY(chatterinoIvr, "chatterino.ivr");
Q_LOGGING_CATEGORY(chatterinoMain, "chatterino.main");
Q_LOGGING_CATEGORY(chatterinoMessage, "chatterino.message");
Q_LOGGING_CATEGORY(chatterinoNativeMessage, "chatterino.nativemessage");
Q_LOGGING_CATEGORY(chatterinoNotification, "chatterino.notification");
Q_LOGGING_CATEGORY(chatterinoNuulsuploader, "chatterino.nuulsuploader");
Q_LOGGING_CATEGORY(chatterinoPubsub, "chatterino.pubsub");
Q_LOGGING_CATEGORY(chatterinoStreamlink, "chatterino.streamlink");
Q_LOGGING_CATEGORY(chatterinoTokenizer, "chatterino.tokenizer");
Q_LOGGING_CATEGORY(chatterinoTwitch, "chatterino.twitch");
Q_LOGGING_CATEGORY(chatterinoUpdate, "chatterino.update");
Q_LOGGING_CATEGORY(chatterinoWebsocket, "chatterino.websocket");
Q_LOGGING_CATEGORY(chatterinoWidget, "chatterino.widget");
Q_LOGGING_CATEGORY(chatterinoWindowmanager, "chatterino.windowmanager");
#endif
