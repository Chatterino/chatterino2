/** @noSelfInFile */

declare namespace c2 {
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

        add_message(
            message: Message,
            context?: MessageContext,
            override_flags?: MessageFlag | null
        ): void;

        is_twitch_channel(): boolean;

        get_room_modes(): RoomModes;
        get_stream_status(): StreamStatus;
        get_twitch_id(): string;
        is_broadcaster(): boolean;
        is_mod(): boolean;
        is_vip(): boolean;

        static by_name(this: void, name: string): null | Channel;
        static by_twitch_id(this: void, id: string): null | Channel;
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

        static create(this: void, method: HTTPMethod, url: string): HTTPRequest;
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

    interface WebSocket {
        close(): void;
        send_text(data: string): void;
        send_binary(data: string): void;
        on_open: null | (() => void);
        on_close: null | (() => void);
        on_text: null | ((data: string) => void);
        on_binary: null | ((data: string) => void);
    }
    interface WebSocketConstructor {
        new: (
            this: void,
            url: string,
            options?: {
                headers?: Record<string, string>;
                on_open?: () => void;
                on_close?: () => void;
                on_text?: (data: string) => void;
                on_binary?: (data: string) => void;
            }
        ) => WebSocket;
    }
    var WebSocket: WebSocketConstructor;

    interface Message {
        flags: MessageFlag;
        id: string;
        parse_time: number;
        search_text: string;
        message_text: string;
        login_name: string;
        display_name: string;
        localized_name: string;
        user_id: string;
        channel_name: string;
        username_color: string;
        server_received_time: number;
        highlight_color: string | null;
        frozen: boolean;
        elements(): MessageElement[];
        append_element(init: MessageElementInit): void;
    }

    interface MessageConstructor {
        new: (this: void, init: MessageInit) => Message;
    }
    var Message: MessageConstructor;

    interface MessageInit {
        flags?: MessageFlag;
        id?: string;
        parse_time?: number;
        search_text?: string;
        message_text?: string;
        login_name?: string;
        display_name?: string;
        localized_name?: string;
        user_id?: string;
        channel_name?: string;
        username_color?: string;
        server_received_time?: number;
        highlight_color?: string | null;
        elements?: MessageElementInit[];
    }

    interface MessageElementBase {
        flags: MessageElementFlag;
        tooltip: string;
        trailing_space: boolean;
        link: Link;
        add_flags(flags: MessageElementFlag): void;
    }

    interface MessageElementInitBase {
        tooltip?: string;
        trailing_space?: boolean;
        link?: Link;
    }

    type MessageColor = "text" | "link" | "system" | string;

    type MessageElement =
        | TextElement
        | SingleLineTextElement
        | MentionElement
        | TimestampElement
        | TwitchModerationElement
        | LinebreakElement
        | ReplyCurveElement
        | LinkElement
        | EmoteElement
        | LayeredEmoteElement
        | ImageElement
        | CircularImageElement
        | ScalingImageElement
        | BadgeElement
        | ModBadgeElement
        | VipBadgeElement
        | FfzBadgeElement;

    type MessageElementInit =
        | TextElementInit
        | SingleLineTextElementInit
        | MentionElementInit
        | TimestampElementInit
        | TwitchModerationElementInit
        | LinebreakElementInit
        | ReplyCurveElementInit;

    interface TextElement extends MessageElementBase {
        type: "text";
        words: string[];
        color: string;
        style: c2.FontStyle;
    }

    interface TextElementInit extends MessageElementInitBase {
        type: "text";
        text: string;
        flags?: MessageElementFlag;
        color?: MessageColor;
        style?: FontStyle;
    }

    interface SingleLineTextElement extends MessageElementBase {
        type: "single-line-text";
        words: string[];
        color: string;
        style: c2.FontStyle;
    }

    interface SingleLineTextElementInit extends MessageElementInitBase {
        type: "single-line-text";
        text: string;
        flags?: MessageElementFlag;
        color?: MessageColor;
        style?: FontStyle;
    }

    interface MentionElement extends Omit<TextElement, "type"> {
        type: "mention";
        display_name: string;
        login_name: string;
        fallback_color: string;
        user_color: string;
    }

    interface MentionElementInit extends MessageElementInitBase {
        type: "mention";
        display_name: string;
        login_name: string;
        fallback_color: MessageColor;
        user_color: MessageColor;
    }

    interface TimestampElement extends MessageElementBase {
        type: "timestamp";
        time: number;
    }

    interface TimestampElementInit extends MessageElementInitBase {
        type: "timestamp";
        time?: number;
    }

    interface TwitchModerationElement extends MessageElementBase {
        type: "twitch-moderation";
    }

    interface TwitchModerationElementInit extends MessageElementInitBase {
        type: "twitch-moderation";
    }

    interface LinebreakElement extends MessageElementBase {
        type: "linebreak";
    }

    interface LinebreakElementInit extends MessageElementInitBase {
        type: "linebreak";
        flags?: MessageElementFlag;
    }

    interface ReplyCurveElement extends MessageElementBase {
        type: "reply-curve";
    }

    interface ReplyCurveElementInit extends MessageElementInitBase {
        type: "reply-curve";
    }

    interface LinkElement extends Omit<TextElement, "type"> {
        type: "link";
        lowercase: string;
        original: string;
    }

    interface EmoteElement extends MessageElementBase {
        type: "emote";
    }

    interface LayeredEmoteElement extends MessageElementBase {
        type: "layered-emote";
    }

    interface ImageElement extends MessageElementBase {
        type: "image";
    }

    interface CircularImageElement extends MessageElementBase {
        type: "circular-image";
    }

    interface ScalingImageElement extends MessageElementBase {
        type: "scaling-image";
    }

    interface BadgeElement extends MessageElementBase {
        type: "badge";
    }

    interface ModBadgeElement extends Omit<BadgeElement, "type"> {
        type: "mod-badge";
    }

    interface VipBadgeElement extends Omit<BadgeElement, "type"> {
        type: "ffz-badge";
    }

    interface FfzBadgeElement extends Omit<BadgeElement, "type"> {
        type: "ffz-badge";
    }

    interface Link {
        type: LinkType;
        value: string;
    }

    enum LinkType {
        Url,
        UserInfo,
        UserAction,
        JumpToChannel,
        CopyToClipboard,
        JumpToMessage,
        InsertText,
    }

    enum MessageFlag {
        None = 0,
        System = 0,
        Timeout = 0,
        Highlighted = 0,
        DoNotTriggerNotification = 0,
        Centered = 0,
        Disabled = 0,
        DisableCompactEmotes = 0,
        Collapsed = 0,
        ConnectedMessage = 0,
        DisconnectedMessage = 0,
        Untimeout = 0,
        PubSub = 0,
        Subscription = 0,
        DoNotLog = 0,
        AutoMod = 0,
        RecentMessage = 0,
        Whisper = 0,
        HighlightedWhisper = 0,
        Debug = 0,
        Similar = 0,
        RedeemedHighlight = 0,
        RedeemedChannelPointReward = 0,
        ShowInMentions = 0,
        FirstMessage = 0,
        ReplyMessage = 0,
        ElevatedMessage = 0,
        SubscribedThread = 0,
        CheerMessage = 0,
        LiveUpdatesAdd = 0,
        LiveUpdatesRemove = 0,
        LiveUpdatesUpdate = 0,
        AutoModOffendingMessageHeader = 0,
        AutoModOffendingMessage = 0,
        LowTrustUsers = 0,
        RestrictedMessage = 0,
        MonitoredMessage = 0,
        Action = 0,
        SharedMessage = 0,
        AutoModBlockedTerm = 0,
        ClearChat = 0,
        EventSub = 0,
        ModerationAction = 0,
        InvalidReplyTarget = 0,
    }

    enum MessageElementFlag {
        None = 0,
        Misc = 0,
        Text = 0,
        Username = 0,
        Timestamp = 0,
        TwitchEmoteImage = 0,
        TwitchEmoteText = 0,
        TwitchEmote = 0,
        BttvEmoteImage = 0,
        BttvEmoteText = 0,
        BttvEmote = 0,
        ChannelPointReward = 0,
        ChannelPointRewardImage = 0,
        FfzEmoteImage = 0,
        FfzEmoteText = 0,
        FfzEmote = 0,
        SevenTVEmoteImage = 0,
        SevenTVEmoteText = 0,
        SevenTVEmote = 0,
        EmoteImages = 0,
        EmoteText = 0,
        BitsStatic = 0,
        BitsAnimated = 0,
        BadgeSharedChannel = 0,
        BadgeGlobalAuthority = 0,
        BadgePredictions = 0,
        BadgeChannelAuthority = 0,
        BadgeSubscription = 0,
        BadgeVanity = 0,
        BadgeChatterino = 0,
        BadgeSevenTV = 0,
        BadgeFfz = 0,
        Badges = 0,
        ChannelName = 0,
        BitsAmount = 0,
        ModeratorTools = 0,
        EmojiImage = 0,
        EmojiText = 0,
        EmojiAll = 0,
        AlwaysShow = 0,
        Collapsed = 0,
        Mention = 0,
        LowercaseLinks = 0,
        RepliedMessage = 0,
        ReplyButton = 0,
        Default = 0,
    }

    enum FontStyle {
        Tiny,
        ChatSmall,
        ChatMediumSmall,
        ChatMedium,
        ChatMediumBold,
        ChatMediumItalic,
        ChatLarge,
        ChatVeryLarge,
        TimestampMedium,
        UiMedium,
        UiMediumBold,
        UiTabs,
        EndType,
        ChatStart,
        ChatEnd,
    }

    enum MessageContext {
        Original,
        Repost,
    }

    class TwitchAccount implements IWeakResource {
        is_valid(): boolean;
        user_login(): string;
        user_id(): string;
        color(): string;
        is_anon(): boolean;
    }

    function current_account(): TwitchAccount;
}

declare module "chatterino.json" {
    class _Dummy {}

    function parse(
        text: string,
        opts?: { allow_comments?: boolean; allow_trailing_commas?: boolean }
    ): any;
    function stringify(
        item: any,
        opts?: { pretty?: boolean; indent_char?: string; indent_size?: number }
    ): string;

    let exports: {
        null: _Dummy;
        parse: typeof parse;
        stringify: typeof stringify;
    };
    export = exports;
}
