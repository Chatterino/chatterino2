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
