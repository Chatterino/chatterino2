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
}
