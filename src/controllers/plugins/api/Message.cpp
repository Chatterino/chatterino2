#include "controllers/plugins/api/Message.hpp"

#include "Application.hpp"
#include "messages/MessageElement.hpp"

#ifdef CHATTERINO_HAVE_PLUGINS

#    include "controllers/plugins/SolTypes.hpp"
#    include "messages/Message.hpp"

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

std::unique_ptr<TextElement> textElementFromTable(const sol::table &tbl)
{
    return std::make_unique<TextElement>(
        tbl["text"], tbl.get_or("flags", MessageElementFlag::Text),
        MessageColor::fromLua(tbl.get_or("color", QString{})),
        tbl.get_or("style", FontStyle::ChatMedium));
}

std::unique_ptr<SingleLineTextElement> singleLineTextElementFromTable(
    const sol::table &tbl)
{
    return std::make_unique<SingleLineTextElement>(
        tbl["text"], tbl.get_or("flags", MessageElementFlag::Text),
        MessageColor::fromLua(tbl.get_or("color", QString{})),
        tbl.get_or("style", FontStyle::ChatMedium));
}

std::unique_ptr<MentionElement> mentionElementFromTable(const sol::table &tbl)
{
    // no flags!
    return std::make_unique<MentionElement>(
        tbl.get<QString>("display_name"), tbl.get<QString>("login_name"),
        MessageColor::fromLua(tbl.get<QString>("fallback_color")),
        MessageColor::fromLua(tbl.get<QString>("user_color")));
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

std::unique_ptr<MessageElement> elementFromTable(const sol::table &tbl)
{
    QString type = tbl["type"];
    std::unique_ptr<MessageElement> el;
    if (type == u"text")
    {
        el = textElementFromTable(tbl);
    }
    else if (type == u"single-line-text")
    {
        el = singleLineTextElementFromTable(tbl);
    }
    else if (type == u"mention")
    {
        el = mentionElementFromTable(tbl);
    }
    else if (type == u"timestamp")
    {
        el = timestampElementFromTable(tbl);
    }
    else if (type == u"twitch-moderation")
    {
        el = twitchModerationElementFromTable();
    }
    else if (type == u"linebreak")
    {
        el = linebreakElementFromTable(tbl);
    }
    else if (type == u"reply-curve")
    {
        el = replyCurveElementFromTable();
    }
    else
    {
        throw std::runtime_error("Invalid message type");
    }
    assert(el);

    el->setTrailingSpace(tbl.get_or("trailing_space", true));
    el->setTooltip(tbl.get_or("tooltip", QString{}));

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
        if (!msg || this->index >= this->msg->elements.size())
        {
            return nullptr;
        }
        return this->msg->elements[this->index].get();
    }

    MessageElement &ref() const
    {
        auto *el = element();
        if (!el)
        {
            throw std::runtime_error("Element does not exist");
        }
        return *el;
    }

    template <typename T>
    T &as() const
    {
        auto *el = dynamic_cast<T *>(element());
        if (!el)
        {
            throw std::runtime_error(
                "Element is of invalid type or does not exist");
        }
        return *el;
    }

    template <typename... T>
    auto visit(auto &&...cb) const
    {
        static_assert(sizeof...(T) == sizeof...(cb) && sizeof...(T) > 0);
        return visitOne<T...>(std::forward<decltype(cb)>(cb)...);
    }

    bool operator==(const ElementRef &rhs) const
    {
        return this->msg.get() == rhs.msg.get() && this->index == rhs.index;
    }

    std::shared_ptr<Message> msg;
    size_t index = 0;

private:
    template <typename T, typename... Rest>
    decltype(auto) visitOne(auto &&cb, auto &&...rest) const
    {
        auto *el = dynamic_cast<T *>(element());
        if (!el)
        {
            if constexpr (sizeof...(rest) == 0)
            {
                throw std::runtime_error(
                    "Element is of invalid type or does not exist");
            }
            else
            {
                return visitOne<Rest...>(std::forward<decltype(rest)>(rest)...);
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
    ElementIterator operator++(int)
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
    ElementIterator operator--(int)
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
        return {msg, 0};
    }
    ElementIterator end() const
    {
        return {msg, msg->elements.size()};
    }

    size_type size() const
    {
        if (!msg)
        {
            return 0;
        }
        return msg->elements.size();
    }

    // NOLINTNEXTLINE
    size_type max_size() const
    {
        return this->size();  // we can't insert
    }

    bool empty() const
    {
        if (!msg)
        {
            return true;
        }
        return msg->elements.empty();
    }

    // NOLINTNEXTLINE
    void push_back(ElementIterator::value_type /* v */) const
    {
        throw std::runtime_error("Insertion is not supported");
    }

    // NOLINTNEXTLINE
    void erase(ElementIterator it) const
    {
        if (it.current.msg != msg || !msg ||
            it.current.index >= msg->elements.size())
        {
            throw std::runtime_error("Can't erase here");
        }
        msg->elements.erase(msg->elements.begin() +
                            static_cast<ptrdiff_t>(it.current.index));
    }

    std::shared_ptr<Message> msg;
};

void createUserType(sol::table &c2)
{
    c2.new_usertype<ElementRef>(
        "MessageElement", sol::no_constructor, "type",
        sol::property([](const ElementRef &el) {
            return el.ref().type();
        }),
        "flags", sol::property([](const ElementRef &el) {
            return el.ref().getFlags();
        }),
        "add_flags",
        [](const ElementRef &el, MessageElementFlag flag) {
            el.ref().addFlags(flag);
        },
        "tooltip",
        sol::property(
            [](const ElementRef &el) {
                return el.ref().getTooltip();
            },
            [](const ElementRef &el, const QString &tooltip) {
                el.ref().setTooltip(tooltip);
            }),
        "trailing_space",
        sol::property(
            [](const ElementRef &el) {
                return el.ref().hasTrailingSpace();
            },
            [](const ElementRef &el, bool trailingSpace) {
                el.ref().setTrailingSpace(trailingSpace);
            }),
        "padding", sol::property([](const ElementRef &el) {
            return el.as<CircularImageElement>().padding();
        }),
        "background", sol::property([](const ElementRef &el) {
            return el.as<CircularImageElement>().background();
        }),
        "words", sol::property([](const ElementRef &el) {
            return el.visit<TextElement, SingleLineTextElement>(
                [](const TextElement &el) {
                    return el.words();
                },
                [](const SingleLineTextElement &el) {
                    return el.words();
                });
        }),
        "color", sol::property([](const ElementRef &el) {
            return el.visit<TextElement, SingleLineTextElement>(
                [](const TextElement &el) {
                    return el.color().toLua();
                },
                [](const SingleLineTextElement &el) {
                    return el.color().toLua();
                });
        }),
        "style", sol::property([](const ElementRef &el) {
            return el.visit<TextElement, SingleLineTextElement>(
                [](const TextElement &el) {
                    return el.fontStyle();
                },
                [](const SingleLineTextElement &el) {
                    return el.fontStyle();
                });
        }),
        "lowercase", sol::property([](const ElementRef &el) {
            return el.as<LinkElement>().lowercase();
        }),
        "original", sol::property([](const ElementRef &el) {
            return el.as<LinkElement>().original();
        }),
        "fallback_color", sol::property([](const ElementRef &el) {
            return el.as<MentionElement>().fallbackColor().toLua();
        }),
        "user_color", sol::property([](const ElementRef &el) {
            return el.as<MentionElement>().userColor().toLua();
        }),
        "user_login_name", sol::property([](const ElementRef &el) {
            return el.as<MentionElement>().userLoginName();
        }),
        "time", sol::property([](const ElementRef &el) {
            return el.as<TimestampElement>().time();
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
                msg->flags = f;
            }),
        "parse_time",
        sol::property(
            [](Message *msg) {
                return QDateTime(QDate::currentDate(), msg->parseTime)
                    .toMSecsSinceEpoch();
            },
            [](Message *msg, qint64 ms) {
                msg->parseTime = datetimeFromOffset(ms).time();
            }),
        "id", &Message::id,                         //
        "search_text", &Message::searchText,        //
        "message_text", &Message::messageText,      //
        "login_name", &Message::loginName,          //
        "display_name", &Message::displayName,      //
        "localized_name", &Message::localizedName,  //
        "user_id", &Message::userID,                //
        "channel_name", &Message::channelName,      //
        "username_color",
        sol::property(
            [](Message *msg) {
                return msg->usernameColor.name(QColor::HexArgb);
            },
            [](Message *msg, std::string_view sv) {
                msg->usernameColor = QColor::fromString(sv);
            }),
        "server_received_time",
        sol::property(
            [](Message *msg) {
                return msg->serverReceivedTime.toMSecsSinceEpoch();
            },
            [](Message *msg, qint64 ms) {
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
        "elements", [](const std::shared_ptr<Message> &msg) {
            if (!msg)
            {
                throw std::runtime_error("No message instance");
            }
            return MessageElements(msg);
        });
}

}  // namespace chatterino::lua::api::message

#endif
