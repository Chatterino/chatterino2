/** @noSelfInFile */

declare module c2 {
    enum LogLevel {
        Debug,
        Info,
        Warning,
        Critical,
    }
    enum EventType {
        CompletionRequested = "CompletionRequested",
        Test = "Test",
    }
    enum CompletionType {
        Username,

        // emotes
        EmoteStart,
        FFZGlobalEmote,
        FFZChannelEmote,
        BTTVGlobalEmote,
        BTTVChannelEmote,
        SeventvGlobalEmote,
        SeventvChannelEmote,
        TwitchGlobalEmote,
        TwitchLocalEmote,
        TwitchSubscriberEmote,
        Emoji,
        EmoteEnd,
        // end emotes

        CustomCommand,
        ChatterinoCommand,
        TwitchCommand,
        PluginCommand,
        CustomCompletion,
    }

    class CommandContext {
        words: String[];
        channel_name: String;
    }

    function log(level: LogLevel, ...data: any[]): void;
    function register_command(
        name: String,
        handler: (ctx: CommandContext) => void
    ): boolean;

    type CompletionList = Array<[string, CompletionType]> | {done: boolean};

    type CallbackTypeCompletions = (text: string, prefix: string, is_first_word: boolean) => CompletionList;
    type CallbackTypeTest = (xd: boolean) => void;
    type CallbackType<T> = T extends EventType.CompletionRequested ? CallbackTypeCompletions :
        T extends EventType.Test ? CallbackTypeTest : never;

    function register_callback<Type>(
        type: Type,
        handler: CallbackType<Type>,
    ): void;
    function send_msg(channel: String, text: String): boolean;
    function system_msg(channel: String, text: String): boolean;
}
