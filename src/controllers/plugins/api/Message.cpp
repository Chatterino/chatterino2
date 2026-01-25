// SPDX-FileCopyrightText: 2025 Contributors to Chatterino <https://chatterino.com>
//
// SPDX-License-Identifier: MIT

#ifdef CHATTERINO_HAVE_PLUGINS
#    include "controllers/plugins/api/Message.hpp"

#    include "Application.hpp"
#    include "controllers/plugins/LuaUtilities.hpp"
#    include "controllers/plugins/SolTypes.hpp"
#    include "messages/Message.hpp"
#    include "messages/MessageElement.hpp"

#    include <sol/sol.hpp>

namespace {

using namespace chatterino;

QDateTime datetimeFromOffset(qint64 offset)
{
    auto dt = QDateTime::fromMSecsSinceEpoch(offset);

#    ifdef CHATTERINO_WITH_TESTS
    if (getApp()->isTest())
    {
        return dt.toUTC();
    }
#    endif

    return dt;
}

template <typename T>
T requiredGet(const sol::table &tbl, auto &&key)
{
    auto v = tbl.get<sol::optional<T>>(std::forward<decltype(key)>(key));
    if (!v)
    {
        throw std::runtime_error(std::string{"Missing required property: "} +
                                 key);
    }
    return *std::move(v);
}

std::unique_ptr<TextElement> textElementFromTable(const sol::table &tbl)
{
    return std::make_unique<TextElement>(
        requiredGet<QString>(tbl, "text"),
        tbl.get_or("flags", MessageElementFlag::Text),
        MessageColor::fromLua(tbl.get_or("color", QString{})),
        tbl.get_or("style", FontStyle::ChatMedium));
}

std::unique_ptr<SingleLineTextElement> singleLineTextElementFromTable(
    const sol::table &tbl)
{
    return std::make_unique<SingleLineTextElement>(
        requiredGet<QString>(tbl, "text"),
        tbl.get_or("flags", MessageElementFlag::Text),
        MessageColor::fromLua(tbl.get_or("color", QString{})),
        tbl.get_or("style", FontStyle::ChatMedium));
}

std::unique_ptr<MentionElement> mentionElementFromTable(const sol::table &tbl)
{
    // no flags!
    return std::make_unique<MentionElement>(
        requiredGet<QString>(tbl, "display_name"),
        requiredGet<QString>(tbl, "login_name"),
        MessageColor::fromLua(requiredGet<QString>(tbl, "fallback_color")),
        MessageColor::fromLua(requiredGet<QString>(tbl, "user_color")));
}

std::unique_ptr<TimestampElement> timestampElementFromTable(
    const sol::table &tbl)
{
    // no flags!
    auto time = tbl.get<std::optional<qint64>>("time");
    if (time)
    {
        return std::make_unique<TimestampElement>(
            datetimeFromOffset(*time).time());
    }
    return std::make_unique<TimestampElement>();
}

std::unique_ptr<TwitchModerationElement> twitchModerationElementFromTable()
{
    // no flags!
    return std::make_unique<TwitchModerationElement>();
}

std::unique_ptr<LinebreakElement> linebreakElementFromTable(
    const sol::table &tbl)
{
    return std::make_unique<LinebreakElement>(
        tbl.get_or("flags", MessageElementFlag::None));
}

std::unique_ptr<ReplyCurveElement> replyCurveElementFromTable()
{
    // no flags!
    return std::make_unique<ReplyCurveElement>();
}

void setLinkOn(MessageElement *el, const Link &link)
{
    el->setLink(link);
    QString tooltip;

    switch (link.type)
    {
        case Link::Url:
            tooltip = QString("<b>URL:</b> %1").arg(link.value);
            break;
        case Link::UserAction:
            tooltip = QString("<b>Command:</b> %1").arg(link.value);
            break;
        case Link::CopyToClipboard:
            tooltip = "<b>Copy to clipboard</b>";
            break;

        // these links should be safe to click as they don't have any immediate action associated with them
        case Link::InsertText:
        case Link::JumpToChannel:
        case Link::JumpToMessage:
        case Link::UserInfo:
        case Link::UserWhisper:
        case Link::ReplyToMessage:
            break;

        // these types are not exposed to plugins
        case Link::None:
        case Link::AutoModAllow:
        case Link::AutoModDeny:
        case Link::OpenAccountsPage:
        case Link::Reconnect:
        case Link::ViewThread:
            throw std::runtime_error("Invalid link type. How'd this happen?");
    }
    el->setTooltip(tooltip);
}

std::unique_ptr<MessageElement> elementFromTable(const sol::table &tbl)
{
    auto type = requiredGet<std::string>(tbl, "type");
    std::unique_ptr<MessageElement> el;
    bool linksAllowed = true;
    if (type == TextElement::TYPE)
    {
        el = textElementFromTable(tbl);
    }
    else if (type == SingleLineTextElement::TYPE)
    {
        el = singleLineTextElementFromTable(tbl);
    }
    else if (type == MentionElement::TYPE)
    {
        el = mentionElementFromTable(tbl);
        linksAllowed = false;
    }
    else if (type == TimestampElement::TYPE)
    {
        el = timestampElementFromTable(tbl);
    }
    else if (type == TwitchModerationElement::TYPE)
    {
        el = twitchModerationElementFromTable();
    }
    else if (type == LinebreakElement::TYPE)
    {
        el = linebreakElementFromTable(tbl);
    }
    else if (type == ReplyCurveElement::TYPE)
    {
        el = replyCurveElementFromTable();
        linksAllowed = false;
    }
    else
    {
        throw std::runtime_error("Invalid message type");
    }
    assert(el);

    el->setTrailingSpace(tbl.get_or("trailing_space", true));

    auto link = tbl.get<sol::optional<Link>>("link");
    if (link)
    {
        if (!linksAllowed)
        {
            throw std::runtime_error("'link' not supported on type='" + type +
                                     '\'');
        }
        setLinkOn(el.get(), *link);
    }
    else
    {
        el->setTooltip(tbl.get_or("tooltip", QString{}));
    }

    return el;
}

std::shared_ptr<Message> messageFromTable(const sol::table &tbl)
{
    auto msg = std::make_shared<Message>();
    msg->flags = tbl.get_or("flags", MessageFlag::None);

    // This takes a UTC offset (not the milliseconds since the start of the day)
    auto parseTime = tbl.get<std::optional<qint64>>("parse_time");
    if (parseTime)
    {
        msg->parseTime = datetimeFromOffset(*parseTime).time();
    }

    msg->id = tbl.get_or("id", QString{});
    msg->searchText = tbl.get_or("search_text", QString{});
    msg->messageText = tbl.get_or("message_text", QString{});
    msg->loginName = tbl.get_or("login_name", QString{});
    msg->displayName = tbl.get_or("display_name", QString{});
    msg->localizedName = tbl.get_or("localized_name", QString{});
    msg->userID = tbl.get_or("user_id", QString{});
    // missing: timeoutUser
    msg->channelName = tbl.get_or("channel_name", QString{});

    auto usernameColor = tbl.get_or("username_color", QString{});
    if (!usernameColor.isEmpty())
    {
        msg->usernameColor = QColor(usernameColor);
    }

    auto serverReceivedTime =
        tbl.get<std::optional<qint64>>("server_received_time");
    if (serverReceivedTime)
    {
        msg->serverReceivedTime = datetimeFromOffset(*serverReceivedTime);
    }

    // missing: badges
    // missing: badgeInfos

    // we construct a color on the fly here
    auto highlightColor = tbl.get_or("highlight_color", QString{});
    if (!highlightColor.isEmpty())
    {
        msg->highlightColor = std::make_shared<QColor>(highlightColor);
    }

    // missing: replyThread
    // missing: replyParent
    // missing: count

    auto elements = tbl.get<std::optional<sol::table>>("elements");
    if (elements)
    {
        auto size = elements->size();
        for (size_t i = 1; i <= size; i++)
        {
            msg->elements.emplace_back(
                elementFromTable(elements->get<sol::table>(i)));
        }
    }

    // missing: reward
    return msg;
}

void checkWritable(Message *msg)
{
    if (msg->frozen)
    {
        throw std::runtime_error("Message is frozen");
    }
}

template <typename T>
struct MemberPtrTraits;

template <class T, class C>
struct MemberPtrTraits<T C::*> {
    using Type = T;
    using Object = C;
};

template <auto T>
decltype(auto) memberAccessor()
{
    return sol::property(
        [](Message *msg) {
            return msg->*T;
        },
        [](Message *msg, typename MemberPtrTraits<decltype(T)>::Type v) {
            checkWritable(msg);
            msg->*T = std::forward<decltype(v)>(v);
        });
}

}  // namespace

namespace chatterino::lua::api::message {

struct ElementRef {
    ElementRef() = default;
    ElementRef(std::shared_ptr<Message> msg, size_t index)
        : msg(std::move(msg))
        , index(index)
    {
    }

    MessageElement *element() const
    {
        if (!this->msg || this->index >= this->msg->elements.size())
        {
            return nullptr;
        }
        checkWritable(this->msg.get());
        return this->msg->elements[this->index].get();
    }

    const MessageElement *constElement() const
    {
        if (!this->msg || this->index >= this->msg->elements.size())
        {
            return nullptr;
        }
        return this->msg->elements[this->index].get();
    }

    MessageElement &ref() const
    {
        auto *el = this->element();
        if (!el)
        {
            throw std::runtime_error("Element does not exist or expired");
        }
        return *el;
    }

    const MessageElement &cref() const
    {
        const auto *el = this->constElement();
        if (!el)
        {
            throw std::runtime_error("Element does not exist or expired");
        }
        return *el;
    }

    /// Cast this element to `T`. Otherwise nullopt is returned.
    /// Use `.map()` to access the content.
    template <typename T>
    sol::optional<T &> as() const
    {
        // using ref() to error if the reference is invalid
        auto *el = dynamic_cast<T *>(&this->ref());
        if (!el)
        {
            return sol::nullopt;
        }
        return *el;
    }

    /// Cast this element to `const T`. Otherwise nullopt is returned.
    /// Use `.map()` to access the content.
    template <typename T>
    sol::optional<const T &> asConst() const
    {
        // using cref() to error if the reference is invalid
        const auto *el = dynamic_cast<const T *>(&this->cref());
        if (!el)
        {
            return sol::nullopt;
        }
        return *el;
    }

    template <typename T>
    bool is() const
    {
        return dynamic_cast<const T *>(&this->cref()) != nullptr;
    }

    /// Visit this element by dynamic casting
    template <typename... T>
    auto visit(auto &&...cb) const
    {
        static_assert(sizeof...(T) == sizeof...(cb) && sizeof...(T) > 0);

        // infer the returned type inside the optional
        using Cb0 = std::tuple_element_t<0, std::tuple<decltype(cb)...>>;
        using T0 = std::tuple_element_t<0, std::tuple<T...>>;
        using TReturn = std::invoke_result_t<Cb0, T0 &>;

        return this->visitOne<TReturn, T...>(std::forward<decltype(cb)>(cb)...);
    }

    bool operator==(const ElementRef &rhs) const
    {
        return this->msg.get() == rhs.msg.get() && this->index == rhs.index;
    }

    std::shared_ptr<Message> msg;
    size_t index = 0;

private:
    template <bool Const>
    decltype(auto) maybeConstElement() const
    {
        if constexpr (Const)
        {
            return this->constElement();
        }
        else
        {
            return this->element();
        }
    }

    /// Run one callback
    ///
    /// This is called recursively.
    /// If the callback returns something, we return an `optional<T>` otherwise
    /// we return `void`.
    template <typename TReturn, typename T, typename... Rest>
    auto visitOne(auto &&cb, auto &&...rest) const
        -> std::conditional_t<std::is_void_v<TReturn>, void,
                              sol::optional<TReturn>>
    {
        auto *el =
            dynamic_cast<T *>(this->maybeConstElement<std::is_const_v<T>>());
        if (!el)
        {
            if constexpr (sizeof...(rest) == 0)
            {
                if constexpr (std::is_void_v<
                                  std::invoke_result_t<decltype(cb), T &>>)
                {
                    return;
                }
                else
                {
                    return sol::nullopt;
                }
            }
            else
            {
                return this->visitOne<TReturn, Rest...>(
                    std::forward<decltype(rest)>(rest)...);
            }
        }
        return std::invoke(cb, *el);
    }
};

struct ElementIterator {
    using difference_type = std::ptrdiff_t;
    using value_type = ElementRef;

    ElementIterator() = default;
    ElementIterator(std::shared_ptr<Message> msg, size_t index)
        : current(std::move(msg), index)
    {
    }

    ElementRef operator*() const
    {
        return this->current;
    }
    ElementRef operator[](size_t i) const
    {
        return {this->current.msg, this->current.index + i};
    }

    ElementIterator &operator+=(difference_type diff)
    {
        this->current.index += diff;
        return *this;
    }
    ElementIterator &operator++()
    {
        return *this += 1;
    }
    ElementIterator operator++(int)  // postfix increment
    {
        auto tmp = *this;
        ++*this;
        return tmp;
    }
    friend ElementIterator operator+(difference_type diff,
                                     const ElementIterator &it)
    {
        return {it.current.msg, it.current.index + diff};
    }
    friend ElementIterator operator+(const ElementIterator &it,
                                     difference_type diff)
    {
        return {it.current.msg, it.current.index + diff};
    }

    ElementIterator &operator-=(difference_type diff)
    {
        this->current.index -= diff;
        return *this;
    }
    ElementIterator &operator--()
    {
        return *this -= 1;
    }
    ElementIterator operator--(int)  // postfix decrement
    {
        auto tmp = *this;
        --*this;
        return tmp;
    }

    friend difference_type operator-(const ElementIterator &lhs,
                                     const ElementIterator &rhs)
    {
        return static_cast<difference_type>(lhs.current.index) -
               static_cast<difference_type>(rhs.current.index);
    }
    friend ElementIterator operator-(difference_type diff,
                                     const ElementIterator &it)
    {
        return {it.current.msg, it.current.index - diff};
    }
    friend ElementIterator operator-(const ElementIterator &it,
                                     difference_type diff)
    {
        return {it.current.msg, it.current.index - diff};
    }

    bool operator==(const ElementIterator &rhs) const
    {
        return this->current == rhs.current;
    }
    bool operator<(const ElementIterator &rhs) const
    {
        return this->current.index < rhs.current.index;
    }
    bool operator<=(const ElementIterator &rhs) const
    {
        return this->current.index <= rhs.current.index;
    }
    bool operator>(const ElementIterator &rhs) const
    {
        return this->current.index > rhs.current.index;
    }
    bool operator>=(const ElementIterator &rhs) const
    {
        return this->current.index >= rhs.current.index;
    }

    ElementRef current;
};

static_assert(std::random_access_iterator<ElementIterator>);

struct MessageElements {
    using value_type = ElementIterator::value_type;
    using iterator = ElementIterator;
    using size_type = size_t;

    explicit MessageElements(std::shared_ptr<Message> msg)
        : msg(std::move(msg))
    {
    }
    ~MessageElements() = default;

    MessageElements(const MessageElements &) = delete;
    MessageElements(MessageElements &&) = default;
    MessageElements &operator=(const MessageElements &) = delete;
    MessageElements &operator=(MessageElements &&) = default;

    ElementIterator begin() const
    {
        return {this->msg, 0};
    }
    ElementIterator end() const
    {
        return {this->msg, this->msg->elements.size()};
    }

    size_type size() const
    {
        if (!this->msg)
        {
            return 0;
        }
        return this->msg->elements.size();
    }

    // NOLINTNEXTLINE
    size_type max_size() const
    {
        return this->size();  // we can't insert
    }

    bool empty() const
    {
        if (!this->msg)
        {
            return true;
        }
        return this->msg->elements.empty();
    }

    // NOLINTNEXTLINE
    void push_back(ElementIterator::value_type /* v */) const
    {
        throw std::runtime_error("Insertion is not supported");
    }

    // NOLINTNEXTLINE
    void erase(ElementIterator it) const
    {
        if (it.current.msg != this->msg || !this->msg ||
            it.current.index >= this->msg->elements.size())
        {
            throw std::runtime_error("Can't erase here");
        }
        checkWritable(this->msg.get());
        this->msg->elements.erase(this->msg->elements.begin() +
                                  static_cast<ptrdiff_t>(it.current.index));
    }

    std::shared_ptr<Message> msg;
};

void createUserType(sol::table &c2)
{
    c2.new_usertype<ElementRef>(
        "MessageElement", sol::no_constructor,  //
        "type", sol::property([](const ElementRef &el) {
            return el.cref().type();
        }),
        "flags", sol::property([](const ElementRef &el) {
            return el.cref().getFlags().value();
        }),
        "add_flags",
        [](const ElementRef &el, MessageElementFlag flag) {
            el.ref().addFlags(flag);
        },
        "link",
        sol::property(
            [](const ElementRef &el) {
                return el.cref().getLink();
            },
            [](const ElementRef &el, const Link &link) {
                if (el.is<MentionElement>() || el.is<LinkElement>())
                {
                    throw std::runtime_error(
                        "Setting a link on this element is unsupported");
                }
                setLinkOn(&el.ref(), link);
            }),
        "tooltip",
        sol::property(
            [](const ElementRef &el) {
                return el.cref().getTooltip();
            },
            [](const ElementRef &el, const QString &tooltip) {
                el.ref().setTooltip(tooltip);
            }),
        "trailing_space",
        sol::property(
            [](const ElementRef &el) {
                return el.cref().hasTrailingSpace();
            },
            [](const ElementRef &el, bool trailingSpace) {
                el.ref().setTrailingSpace(trailingSpace);
            }),
        "padding", sol::property([](const ElementRef &el) {
            return el.asConst<CircularImageElement>().map(
                &CircularImageElement::padding);
        }),
        "background", sol::property([](const ElementRef &el) {
            return el.as<CircularImageElement>().map(
                [](const CircularImageElement &el) {
                    return el.background().name(QColor::HexArgb);
                });
        }),
        "words", sol::property([](const ElementRef &el) {
            return el.visit<const TextElement, const SingleLineTextElement>(
                &TextElement::words, &SingleLineTextElement::words);
        }),
        "color", sol::property([](const ElementRef &el) {
            return el.visit<const TextElement, const SingleLineTextElement>(
                [](const TextElement &el) {
                    return el.color().toLua();
                },
                [](const SingleLineTextElement &el) {
                    return el.color().toLua();
                });
        }),
        "style", sol::property([](const ElementRef &el) {
            return el.visit<const TextElement, const SingleLineTextElement>(
                &TextElement::fontStyle, &SingleLineTextElement::fontStyle);
        }),
        "lowercase", sol::property([](const ElementRef &el) {
            return el.asConst<LinkElement>().map(&LinkElement::lowercase);
        }),
        "original", sol::property([](const ElementRef &el) {
            return el.asConst<LinkElement>().map(&LinkElement::original);
        }),
        "fallback_color", sol::property([](const ElementRef &el) {
            return el.asConst<MentionElement>().map(
                [](const MentionElement &el) {
                    return el.fallbackColor().toLua();
                });
        }),
        "user_color", sol::property([](const ElementRef &el) {
            return el.asConst<MentionElement>().map(
                [](const MentionElement &el) {
                    return el.userColor().toLua();
                });
        }),
        "user_login_name", sol::property([](const ElementRef &el) {
            return el.asConst<MentionElement>().map(
                &MentionElement::userLoginName);
        }),
        "time", sol::property([](const ElementRef &el) {
            return el.asConst<TimestampElement>().map(
                [](const TimestampElement &el) {
                    return QDateTime(QDate::currentDate(), el.time())
                        .toMSecsSinceEpoch();
                });
        }));

    c2.new_usertype<Message>(
        "Message", sol::factories([](const sol::table &tbl) {
            return messageFromTable(tbl);
        }),
        "flags",
        sol::property(
            [](Message *msg) {
                return msg->flags.value();
            },
            [](Message *msg, MessageFlag f) {
                // flags are always mutable
                msg->flags = f;
            }),
        "parse_time",
        sol::property(
            [](Message *msg) {
                return QDateTime(QDate::currentDate(), msg->parseTime)
                    .toMSecsSinceEpoch();
            },
            [](Message *msg, qint64 ms) {
                checkWritable(msg);
                msg->parseTime = datetimeFromOffset(ms).time();
            }),
        "id", memberAccessor<&Message::id>(),                         //
        "search_text", memberAccessor<&Message::searchText>(),        //
        "message_text", memberAccessor<&Message::messageText>(),      //
        "login_name", memberAccessor<&Message::loginName>(),          //
        "display_name", memberAccessor<&Message::displayName>(),      //
        "localized_name", memberAccessor<&Message::localizedName>(),  //
        "user_id", memberAccessor<&Message::userID>(),                //
        "channel_name", memberAccessor<&Message::channelName>(),      //
        "username_color",
        sol::property(
            [](Message *msg) {
                return msg->usernameColor.name(QColor::HexArgb);
            },
            [](Message *msg, std::string_view sv) {
                checkWritable(msg);
                msg->usernameColor = QColor::fromString(sv);
            }),
        "server_received_time",
        sol::property(
            [](Message *msg) {
                return msg->serverReceivedTime.toMSecsSinceEpoch();
            },
            [](Message *msg, qint64 ms) {
                checkWritable(msg);
                msg->serverReceivedTime = datetimeFromOffset(ms);
            }),
        "highlight_color",
        sol::property(
            [](Message *msg) {
                if (!msg->highlightColor)
                {
                    return QString{};
                }
                return msg->highlightColor->name(QColor::HexArgb);
            },
            [](Message *msg, std::string_view sv) {
                checkWritable(msg);
                if (sv.empty())
                {
                    msg->highlightColor.reset();
                }
                else
                {
                    msg->highlightColor =
                        std::make_shared<QColor>(QColor::fromString(sv));
                }
            }),
        // must be read only (but it might be helpful for generic Lua functions)
        "frozen", sol::property([](Message *msg) {
            return msg->frozen;
        }),
        "elements",
        [](const std::shared_ptr<Message> &msg) {
            return MessageElements(msg);
        },
        "append_element",
        [](Message *msg, const sol::table &tbl) {
            checkWritable(msg);
            auto el = elementFromTable(tbl);
            if (el)
            {
                msg->elements.emplace_back(std::move(el));
            }
        });
}

}  // namespace chatterino::lua::api::message

#endif
