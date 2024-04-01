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
    channel: Channel;
  }

  enum Platform {
    Twitch,
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
    Misc,
  }

  interface IWeakResource {
    is_valid(): boolean;
  }

  class RoomModes {
    unique_chat: boolean;
    subscriber_only: boolean;
    emotes_only: boolean;
    follower_only: null | number;
    slow_mode: null | number;
  }
  class StreamStatus {
    live: boolean;
    viewer_count: number;
    uptime: number;
    title: string;
    game_name: string;
    game_id: string;
  }

  class Channel implements IWeakResource {
    is_valid(): boolean;
    get_name(): string;
    get_type(): ChannelType;
    get_display_name(): string;
    send_message(message: string, execute_commands: boolean): void;
    add_system_message(message: string): void;

    is_twitch_channel(): boolean;

    get_room_modes(): RoomModes;
    get_stream_status(): StreamStatus;
    get_twitch_id(): string;
    is_broadcaster(): boolean;
    is_mod(): boolean;
    is_vip(): boolean;

    static by_name(name: string, platform: Platform): null | Channel;
    static by_twitch_id(id: string): null | Channel;
  }

  function log(level: LogLevel, ...data: any[]): void;
  function register_command(
    name: String,
    handler: (ctx: CommandContext) => void
  ): boolean;

  class CompletionEvent {
    query: string;
    full_text_content: string;
    cursor_position: number;
    is_first_word: boolean;
  }

  class CompletionList {
    values: String[];
    hide_others: boolean;
  }

  enum EventType {
    CompletionRequested = "CompletionRequested",
  }

  type CbFuncCompletionsRequested = (ev: CompletionEvent) => CompletionList;
  type CbFunc<T> = T extends EventType.CompletionRequested
    ? CbFuncCompletionsRequested
    : never;

  function register_callback<T>(type: T, func: CbFunc<T>): void;
  function later(callback: () => void, msec: number): void;
}
