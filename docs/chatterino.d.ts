/** @noSelfInFile */

declare module c2 {
  enum LogLevel {
    Debug,
    Info,
    Warning,
    Critical,
  }
  class CommandContext {
    words: String[];
    channel_name: String;
  }

  enum Platform {
    Twitch,
    IRC
  }
  enum ChannelType {
    None,
    Direct,
    Twitch,
    TwitchWhispers,
    TwitchWatching,
    TwitchMentions,
    TwitchLive,
    TwitchAutomod,
    Irc,
    Misc
  }

  interface IWeakResource {
    is_valid(): boolean;
  }

  class Channel implements IWeakResource {
    is_valid(): boolean;
    get_name(): string;
    get_type(): ChannelType;
    get_display_name(): string;
    is_twitch_channel(): boolean;

    static by_name(name: string, platform: Platform): null|Channel;
    static by_twitch_id(id: string): null|Channel;
  }


  function log(level: LogLevel, ...data: any[]): void;
  function register_command(
    name: String,
    handler: (ctx: CommandContext) => void
  ): boolean;
  function send_msg(channel: String, text: String): boolean;
  function system_msg(channel: String, text: String): boolean;

  class CompletionList {
    values: String[];
    hide_others: boolean;
  }

  enum EventType {
    CompletionRequested = "CompletionRequested",
  }

  type CbFuncCompletionsRequested = (
    query: string,
    full_text_content: string,
    cursor_position: number,
    is_first_word: boolean
  ) => CompletionList;
  type CbFunc<T> = T extends EventType.CompletionRequested
    ? CbFuncCompletionsRequested
    : never;

  function register_callback<T>(type: T, func: CbFunc<T>): void;
}
