/** @noSelfInFile */

declare module c2 {
    enum LogLevel {
        Debug,
        Info,
        Warning,
        Critical,
    }
    class CommandContext {
        words: string[];
        channel: Channel;
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
        Misc,
    }

    interface IWeakResource {
        is_valid(): boolean;
    }

    interface ISharedResource {}

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
        send_message(message: string): void;

        add_system_message(message: string): void;

        is_twitch_channel(): boolean;

        get_room_modes(): RoomModes;
        get_stream_status(): StreamStatus;
        get_twitch_id(): string;
        is_broadcaster(): boolean;
        is_mod(): boolean;
        is_vip(): boolean;

        static by_name(name: string): null | Channel;
        static by_twitch_id(id: string): null | Channel;
    }

    enum HTTPMethod {
        Get,
        Post,
        Put,
        Delete,
        Patch,
    }

    class HTTPResponse implements ISharedResource {
        data(): string;
        status(): number | null;
        error(): string;
    }

    type HTTPCallback = (res: HTTPResponse) => void;
    class HTTPRequest implements ISharedResource {
        on_success(callback: HTTPCallback): void;
        on_error(callback: HTTPCallback): void;
        finally(callback: () => void): void;

        set_timeout(millis: number): void;
        set_payload(data: string): void;
        set_header(name: string, value: string): void;

        execute(): void;

        // might error
        static create(method: HTTPMethod, url: string): HTTPRequest;
    }

    function log(level: LogLevel, ...data: any[]): void;
    function register_command(
        name: string,
        handler: (ctx: CommandContext) => void
    ): boolean;

    class CompletionEvent {
        query: string;
        full_text_content: string;
        cursor_position: number;
        is_first_word: boolean;
    }

    class CompletionList {
        values: string[];
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
