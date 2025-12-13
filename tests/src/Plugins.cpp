#include "mocks/Helix.hpp"
#ifdef CHATTERINO_HAVE_PLUGINS
#    include "Application.hpp"
#    include "common/Channel.hpp"
#    include "common/network/NetworkCommon.hpp"
#    include "controllers/accounts/AccountController.hpp"
#    include "controllers/commands/Command.hpp"  // IWYU pragma: keep
#    include "controllers/commands/CommandController.hpp"
#    include "controllers/plugins/api/ChannelRef.hpp"
#    include "controllers/plugins/api/WebSocket.hpp"
#    include "controllers/plugins/Plugin.hpp"
#    include "controllers/plugins/PluginController.hpp"
#    include "controllers/plugins/PluginPermission.hpp"
#    include "controllers/plugins/SolTypes.hpp"  // IWYU pragma: keep
#    include "lib/Snapshot.hpp"
#    include "messages/Message.hpp"
#    include "messages/MessageElement.hpp"
#    include "mocks/BaseApplication.hpp"
#    include "mocks/Channel.hpp"
#    include "mocks/EmoteController.hpp"
#    include "mocks/Logging.hpp"
#    include "mocks/TwitchIrcServer.hpp"
#    include "NetworkHelpers.hpp"
#    include "singletons/Logging.hpp"
#    include "Test.hpp"

#    include <lauxlib.h>
#    include <sol/state_view.hpp>
#    include <sol/table.hpp>

#    include <memory>
#    include <optional>
#    include <utility>

using namespace chatterino;
using chatterino::mock::MockChannel;

namespace {

constexpr bool UPDATE_SNAPSHOTS = false;

const QString TEST_SETTINGS = R"(
{
    "plugins": {
        "supportEnabled": true,
        "enabledPlugins": [
            "test"
        ]
    },
    "accounts": {
        "uid117166826": {
            "username": "testaccount_420",
            "userID": "117166826",
            "clientID": "abc",
            "oauthToken": "def"
        },
        "current": "testaccount_420"
    }
}
)";

class MockTwitch : public mock::MockTwitchIrcServer
{
public:
    ChannelPtr mm2pl = std::make_shared<MockChannel>("mm2pl");

    ChannelPtr getChannelOrEmpty(const QString &dirtyChannelName) override
    {
        if (dirtyChannelName == "mm2pl")
        {
            return this->mm2pl;
        }
        return Channel::getEmpty();
    }

    std::shared_ptr<Channel> getChannelOrEmptyByID(
        const QString &channelID) override
    {
        if (channelID == "117691339")
        {
            return this->mm2pl;
        }
        return Channel::getEmpty();
    }
};

class MockApplication : public mock::BaseApplication
{
public:
    MockApplication()
        : mock::BaseApplication(TEST_SETTINGS)
        , plugins(this->paths_)
        , commands(this->paths_)
    {
    }

    PluginController *getPlugins() override
    {
        return &this->plugins;
    }

    CommandController *getCommands() override
    {
        return &this->commands;
    }

    EmoteController *getEmotes() override
    {
        return &this->emotes;
    }

    mock::MockTwitchIrcServer *getTwitch() override
    {
        return &this->twitch;
    }

    ILogging *getChatLogger() override
    {
        return &this->logging;
    }

    AccountController *getAccounts() override
    {
        return &this->accounts;
    }

    PluginController plugins;
    mock::Logging logging;
    CommandController commands;
    mock::EmoteController emotes;
    MockTwitch twitch;
    AccountController accounts;
    mock::Helix helix;
};

QDir luaTestBaseDir(const QString &category)
{
    QDir snapshotDir(QStringLiteral(__FILE__));
    snapshotDir.cd("../../lua/");
    snapshotDir.cd(category);
    return snapshotDir;
}

QStringList discoverLuaTests(const QString &category)
{
    auto files =
        luaTestBaseDir(category).entryList(QDir::NoDotAndDotDot | QDir::Files);
    for (auto &file : files)
    {
        file.remove(".lua");
    }
    return files;
}

std::string luaTestPath(const QString &category, const QString &entry)
{
    return luaTestBaseDir(category).filePath(entry + ".lua").toStdString();
}

bool runLuaTest(const QString &category, const QString &entry,
                sol::state_view lua)
{
    auto loadResult = lua.load_file(luaTestPath(category, entry));
    auto pfn = loadResult.get<sol::protected_function>();
    pfn.set_error_handler(lua["debug"]["traceback"]);
    auto pfr = pfn.call();
    EXPECT_TRUE(pfr.valid());
    if (!pfr.valid())
    {
        qDebug().noquote() << "Test" << entry << "failed:";
        sol::error err = pfr;
        qDebug().noquote() << err.what();
        return false;
    }
    return true;
}

}  // namespace

namespace chatterino {

class PluginControllerAccess
{
public:
    static bool tryLoadFromDir(const QDir &pluginDir)
    {
        return getApp()->getPlugins()->tryLoadFromDir(pluginDir);
    }

    static void openLibrariesFor(Plugin *plugin)
    {
        getApp()->getPlugins()->openLibrariesFor(plugin);
    }

    static std::map<QString, std::unique_ptr<Plugin>> &plugins()
    {
        return getApp()->getPlugins()->plugins_;
    }

    static lua_State *state(Plugin *pl)
    {
        return pl->state_;
    }
};

}  // namespace chatterino

class PluginTest : public ::testing::Test
{
protected:
    void configure(std::vector<PluginPermission> permissions = {})
    {
        this->app = std::make_unique<MockApplication>();

        auto &plugins = PluginControllerAccess::plugins();
        {
            PluginMeta meta;
            meta.name = "Test";
            meta.license = "MIT";
            meta.homepage = "https://github.com/Chatterino/chatterino2";
            meta.description = "Plugin for tests";
            meta.permissions = std::move(permissions);

            QDir plugindir =
                QDir(app->paths_.pluginsDirectory).absoluteFilePath("test");

            plugindir.mkpath(".");
            auto temp = std::make_unique<Plugin>("test", luaL_newstate(), meta,
                                                 plugindir);
            this->rawpl = temp.get();
            plugins.insert({"test", std::move(temp)});
        }

        // XXX: this skips PluginController::load()
        PluginControllerAccess::openLibrariesFor(rawpl);
        this->lua = new sol::state_view(PluginControllerAccess::state(rawpl));

        this->channel = app->twitch.mm2pl;
        this->rawpl->dataDirectory().mkpath(".");
        initializeHelix(&this->app->helix);
        this->app->accounts.load();
    }

    void TearDown() override
    {
        // perform safe destruction of the plugin
        delete this->lua;
        this->lua = nullptr;
        PluginControllerAccess::plugins().clear();
        this->rawpl = nullptr;
        this->app.reset();
    }

    Plugin *rawpl = nullptr;
    std::unique_ptr<MockApplication> app;
    sol::state_view *lua;
    ChannelPtr channel;
};

TEST_F(PluginTest, testCommands)
{
    configure();

    lua->script(R"lua(
        _G.called = false
        _G.words = nil
        _G.channel = nil
        c2.register_command("/test", function(ctx)
            _G.called = true
            _G.words = ctx.words
            _G.channel = ctx.channel
        end)
    )lua");

    EXPECT_EQ(app->commands.pluginCommands(), QStringList{"/test"});
    app->commands.execCommand("/test with arguments", channel, false);
    bool called = (*lua)["called"];
    EXPECT_EQ(called, true);

    EXPECT_NE((*lua)["words"], sol::nil);
    {
        sol::table tbl = (*lua)["words"];
        std::vector<std::string> words;
        for (auto &o : tbl)
        {
            words.push_back(o.second.as<std::string>());
        }
        EXPECT_EQ(words,
                  std::vector<std::string>({"/test", "with", "arguments"}));
    }

    sol::object chnobj = (*lua)["channel"];
    EXPECT_EQ(chnobj.get_type(), sol::type::userdata);
    lua::api::ChannelRef ref = chnobj.as<lua::api::ChannelRef>();
    EXPECT_EQ(ref.get_name(), channel->getName());
}

TEST_F(PluginTest, testCompletion)
{
    configure();

    lua->script(R"lua(
        _G.called = false
        _G.query = nil
        _G.full_text_content = nil
        _G.cursor_position = nil
        _G.is_first_word = nil

        c2.register_callback(
            c2.EventType.CompletionRequested,
            function(ev)
                _G.called = true
                _G.query = ev.query
                _G.full_text_content = ev.full_text_content
                _G.cursor_position = ev.cursor_position
                _G.is_first_word = ev.is_first_word
                if ev.query == "exclusive" then
                    return {
                        hide_others = true,
                        values = {"Completion1", "Completion2"}
                    }
                end
                return {
                    hide_others = false,
                    values = {"Completion"},
                }
            end
        )
    )lua");

    bool done{};
    QStringList results;
    std::tie(done, results) =
        app->plugins.updateCustomCompletions("foo", "foo", 3, true);
    ASSERT_EQ(done, false);
    ASSERT_EQ(results, QStringList{"Completion"});

    ASSERT_EQ((*lua).get<std::string>("query"), "foo");
    ASSERT_EQ((*lua).get<std::string>("full_text_content"), "foo");
    ASSERT_EQ((*lua).get<int>("cursor_position"), 3);
    ASSERT_EQ((*lua).get<bool>("is_first_word"), true);

    std::tie(done, results) = app->plugins.updateCustomCompletions(
        "exclusive", "foo exclusive", 13, false);
    ASSERT_EQ(done, true);
    ASSERT_EQ(results, QStringList({"Completion1", "Completion2"}));

    ASSERT_EQ((*lua).get<std::string>("query"), "exclusive");
    ASSERT_EQ((*lua).get<std::string>("full_text_content"), "foo exclusive");
    ASSERT_EQ((*lua).get<int>("cursor_position"), 13);
    ASSERT_EQ((*lua).get<bool>("is_first_word"), false);
}

TEST_F(PluginTest, testChannel)
{
    configure();
    lua->script(R"lua(
        chn = c2.Channel.by_name("mm2pl")
    )lua");

    ASSERT_EQ(lua->script(R"lua( return chn:get_name() )lua").get<QString>(0),
              "mm2pl");
    ASSERT_EQ(
        lua->script(R"lua( return chn:get_type() )lua").get<Channel::Type>(0),
        Channel::Type::Twitch);
    ASSERT_EQ(
        lua->script(R"lua( return chn:get_display_name() )lua").get<QString>(0),
        "mm2pl");
    // TODO: send_message, add_system_message

    ASSERT_EQ(
        lua->script(R"lua( return chn:is_twitch_channel() )lua").get<bool>(0),
        true);

    ASSERT_TRUE(lua->script(R"lua(
        assert(c2.Channel.by_name("mm2pl") == c2.Channel.by_name("mm2pl"))
        assert(not c2.Channel.by_name("mm2pl") ~= c2.Channel.by_name("mm2pl"))
        assert(c2.Channel.by_name("mm2pl") ~= c2.Channel.by_name("twitchdev"))
        assert(c2.Channel.by_name("twitchdev") == c2.Channel.by_name("twitchdev"))
        assert(c2.Channel.by_name("twitchdev") == c2.Channel.by_name("other")) -- empty channel
    )lua")
                    .valid());

    // this is not a TwitchChannel
    const auto *shouldThrow1 = R"lua(
        return chn:is_broadcaster()
    )lua";
    EXPECT_ANY_THROW(lua->script(shouldThrow1));
    const auto *shouldThrow2 = R"lua(
        return chn:is_mod()
    )lua";
    EXPECT_ANY_THROW(lua->script(shouldThrow2));
    const auto *shouldThrow3 = R"lua(
        return chn:is_vip()
    )lua";
    EXPECT_ANY_THROW(lua->script(shouldThrow3));
    const auto *shouldThrow4 = R"lua(
        return chn:get_twitch_id()
    )lua";
    EXPECT_ANY_THROW(lua->script(shouldThrow4));
}

TEST_F(PluginTest, testHttp)
{
    {
        PluginPermission net;
        net.type = PluginPermission::Type::Network;
        configure({net});
    }

    lua->script(R"lua(
        function DoReq(url, postdata)
            r = c2.HTTPRequest.create(method, url)
            r:on_success(function(res)
                status = res:status()
                data = res:data()
                error = res:error()
                success = true
            end)
            r:on_error(function(res)
                status = res:status()
                data = res:data()
                error = res:error()
                failure = true
            end)
            r:finally(function()
                finally = true
                done()
            end)
            if postdata ~= "" then
                r:set_payload(postdata)
                r:set_header("Content-Type", "text/plain")
            end
            r:set_timeout(1000)
            r:execute()
        end
    )lua");

    struct RequestCase {
        QString url;
        bool success;
        bool failure;

        int status;
        QString error;

        NetworkRequestType meth = NetworkRequestType::Get;
        QByteArray data;  // null means do not check
    };

    std::vector<RequestCase> cases{
        {"/status/200", true, false, 200, "200"},
        {"/delay/2", false, true, 0, "TimeoutError"},
        {"/post", true, false, 200, "200", NetworkRequestType::Post,
         "Example data"},
    };

    for (const auto &c : cases)
    {
        lua->script(R"lua(
            success = false
            failure = false
            finally = false

            status = nil
            data = nil
            error = nil
        )lua");
        RequestWaiter waiter;
        (*lua)["method"] = c.meth;
        (*lua)["done"] = [&waiter]() {
            waiter.requestDone();
        };

        (*lua)["DoReq"](HTTPBIN_BASE_URL + c.url, c.data);
        waiter.waitForRequest();

        EXPECT_EQ(lua->get<bool>("success"), c.success);
        EXPECT_EQ(lua->get<bool>("failure"), c.failure);
        EXPECT_EQ(lua->get<bool>("finally"), true);

        if (c.status != 0)
        {
            EXPECT_EQ(lua->get<int>("status"), c.status);
        }
        else
        {
            EXPECT_EQ((*lua)["status"], sol::nil);
        }
        EXPECT_EQ(lua->get<QString>("error"), c.error);
        if (!c.data.isNull())
        {
            EXPECT_EQ(lua->get<QByteArray>("data"), c.data);
        }
    }
}

const QByteArray TEST_FILE_DATA = "Test file data\nWith a new line.\n";

TEST_F(PluginTest, ioTest)
{
    {
        PluginPermission ioread;
        PluginPermission iowrite;
        ioread.type = PluginPermission::Type::FilesystemRead;
        iowrite.type = PluginPermission::Type::FilesystemWrite;
        configure({ioread, iowrite});
    }

    lua->set("TEST_DATA", TEST_FILE_DATA);

    lua->script(R"lua(
        f, err = io.open("testfile", "w")
        print(f, err)
        f:write(TEST_DATA)
        f:close()

        f, err = io.open("testfile", "r")
        out = f:read("a")
        f:close()
    )lua");
    EXPECT_EQ(lua->get<QByteArray>("out"), TEST_FILE_DATA);

    lua->script(R"lua(
        io.input("testfile")
        out = io.read("a")
    )lua");
    EXPECT_EQ(lua->get<QByteArray>("out"), TEST_FILE_DATA);

    const auto *shouldThrow1 = R"lua(
        io.popen("/bin/sh", "-c", "notify-send \"This should not execute.\"")
    )lua";
    EXPECT_ANY_THROW(lua->script(shouldThrow1));
    const auto *shouldThrow2 = R"lua(
        io.tmpfile()
    )lua";
    EXPECT_ANY_THROW(lua->script(shouldThrow2));
}

TEST_F(PluginTest, ioNoPerms)
{
    configure();
    auto file = rawpl->dataDirectory().filePath("testfile");
    QFile f(file);
    EXPECT_TRUE(f.open(QFile::WriteOnly));
    f.write(TEST_FILE_DATA);
    f.close();

    EXPECT_EQ(
        // clang-format off
        lua->script(R"lua(
            f, err = io.open("testfile", "r")
            return err
        )lua").get<QString>(0),
       "Plugin does not have permissions to access given file."
        // clang-format on
    );

    const auto *shouldThrow1 = R"lua(
        io.input("testfile")
    )lua";
    EXPECT_ANY_THROW(lua->script(shouldThrow1));

    EXPECT_EQ(
        // clang-format off
        lua->script(R"lua(
            f, err = io.open("testfile", "w")
            return err
        )lua").get<QString>(0),
       "Plugin does not have permissions to access given file."
        // clang-format on
    );

    const auto *shouldThrow2 = R"lua(
        io.output("testfile")
    )lua";
    EXPECT_ANY_THROW(lua->script(shouldThrow2));

    const auto *shouldThrow3 = R"lua(
        io.lines("testfile")
    )lua";
    EXPECT_ANY_THROW(lua->script(shouldThrow3));
}

TEST_F(PluginTest, requireNoData)
{
    {
        PluginPermission ioread;
        PluginPermission iowrite;
        ioread.type = PluginPermission::Type::FilesystemRead;
        iowrite.type = PluginPermission::Type::FilesystemWrite;
        configure({ioread, iowrite});
    }

    auto file = rawpl->dataDirectory().filePath("thisiscode.lua");
    QFile f(file);
    EXPECT_TRUE(f.open(QFile::WriteOnly));
    f.write(R"lua(print("Data was executed"))lua");
    f.close();

    const auto *shouldThrow1 = R"lua(
        require("data.thisiscode")
    )lua";
    EXPECT_ANY_THROW(lua->script(shouldThrow1));
}

TEST_F(PluginTest, testTimerRec)
{
    configure();

    RequestWaiter waiter;
    lua->set("done", [&] {
        waiter.requestDone();
    });

    sol::protected_function fn = lua->script(R"lua(
        local i = 0
        f = function()
            i = i + 1
            c2.log(c2.LogLevel.Info, "cb", i)
            if i < 1024 then
                c2.later(f, 1)
            else
                done()
            end
        end
        c2.later(f, 1)
    )lua");
    waiter.waitForRequest();
}

TEST_F(PluginTest, tryCallTest)
{
    configure();
    lua->script(R"lua(
        function return_table()
            return {
                a="b"
            }
        end
        function return_nothing()
        end
        function return_nil()
            return nil
        end
        function return_nothing_and_error()
            error("I failed :)")
        end
    )lua");

    using func = sol::protected_function;

    func returnTable = lua->get<func>("return_table");
    func returnNil = lua->get<func>("return_nil");
    func returnNothing = lua->get<func>("return_nothing");
    func returnNothingAndError = lua->get<func>("return_nothing_and_error");

    // happy paths
    {
        auto res = lua::tryCall<sol::table>(returnTable);
        EXPECT_TRUE(res.has_value());
        auto t = res.value();
        EXPECT_EQ(t.get<QString>("a"), "b");
    }
    {
        // valid void return
        auto res = lua::tryCall<void>(returnNil);
        EXPECT_TRUE(res.has_value());
    }
    {
        // valid void return
        auto res = lua::tryCall<void>(returnNothing);
        EXPECT_TRUE(res.has_value());
    }
    {
        auto res = lua::tryCall<sol::table>(returnNothingAndError);
        EXPECT_FALSE(res.has_value());
        EXPECT_EQ(res.error(), "[string \"...\"]:13: I failed :)");
    }
    {
        auto res = lua::tryCall<std::optional<int>>(returnNil);
        EXPECT_TRUE(res.has_value());  // no error
        auto opt = *res;
        EXPECT_FALSE(opt.has_value());  // but also no false
    }

    // unhappy paths
    {
        // wrong return type
        auto res = lua::tryCall<int>(returnTable);
        EXPECT_FALSE(res.has_value());
        EXPECT_EQ(res.error(),
                  "Expected int to be returned but table was returned");
    }
    {
        // optional but bad return type
        auto res = lua::tryCall<std::optional<int>>(returnTable);
        EXPECT_FALSE(res.has_value());
        EXPECT_EQ(res.error(), "Expected std::optional<int> to be returned but "
                               "table was returned");
    }
    {
        // no return
        auto res = lua::tryCall<int>(returnNothing);
        EXPECT_FALSE(res.has_value());
        EXPECT_EQ(res.error(),
                  "Expected int to be returned but none was returned");
    }
    {
        // nil return
        auto res = lua::tryCall<int>(returnNil);
        EXPECT_FALSE(res.has_value());
        EXPECT_EQ(res.error(),
                  "Expected int to be returned but lua_nil was returned");
    }
}

TEST_F(PluginTest, testTcpWebSocket)
{
    configure({PluginPermission{{{"type", "Network"}}}});

    RequestWaiter waiter;
    std::vector<std::pair<bool, QByteArray>> messages;
    bool open = false;
    lua->set("done", [&] {
        waiter.requestDone();
    });
    lua->set("add", [&](bool isText, QByteArray data) {
        EXPECT_TRUE(open);
        messages.emplace_back(isText, std::move(data));
    });
    // On GCC in release mode, using set() would cause the done function to be called instead.
    lua->set_function("open", [&] {
        EXPECT_FALSE(open);
        open = true;
    });

    std::shared_ptr<lua::api::WebSocket> ws = lua->script(R"lua(
        local ws = c2.WebSocket.new("ws://127.0.0.1:9052/echo")
        ws.on_text = function(data)
            add(true, data)
        end
        local any_msg = false
        ws.on_binary = function(data)
            if not any_msg then
                any_msg = true
                ws:send_text(string.rep("a", 1 << 15))
                ws:send_binary("wow")
                ws:send_text("/HEADER user-agent")
                ws:send_binary("/CLOSE")
            end
            add(false, data)
        end
        ws.on_close = function()
            done()
        end
        ws.on_open = function()
            open()
        end
        ws:send_text("message1")
        ws:send_text("message2")
        ws:send_text("message3")
        ws:send_binary("message4")

        return ws
    )lua");
    std::weak_ptr<lua::api::WebSocket> weakWs{ws};
    ws.reset();

    waiter.waitForRequest();
    ASSERT_TRUE(open);

    ASSERT_EQ(messages.size(), 7);
    ASSERT_EQ(messages[0].first, true);
    ASSERT_EQ(messages[0].second, "message1");
    ASSERT_EQ(messages[1].first, true);
    ASSERT_EQ(messages[1].second, "message2");
    ASSERT_EQ(messages[2].first, true);
    ASSERT_EQ(messages[2].second, "message3");
    ASSERT_EQ(messages[3].first, false);
    ASSERT_EQ(messages[3].second, "message4");
    ASSERT_EQ(messages[4].first, true);
    ASSERT_EQ(messages[4].second, QByteArray(1 << 15, 'a'));
    ASSERT_EQ(messages[5].first, false);
    ASSERT_EQ(messages[5].second, "wow");
    ASSERT_EQ(messages[6].first, true);
    ASSERT_TRUE(messages[6].second.startsWith("Chatterino"));

    ASSERT_FALSE(weakWs.expired());
    lua->collect_garbage();
    ASSERT_TRUE(weakWs.expired());
}

TEST_F(PluginTest, testTlsWebSocket)
{
    configure({PluginPermission{{{"type", "Network"}}}});

    RequestWaiter waiter;
    bool open = false;
    std::vector<std::pair<bool, QByteArray>> messages;
    lua->set("done", [&] {
        waiter.requestDone();
    });
    lua->set("add", [&](bool isText, QByteArray data) {
        EXPECT_TRUE(open);
        messages.emplace_back(isText, std::move(data));
    });
    // On GCC in release mode, using set() would cause the done function to be called instead.
    lua->set_function("open", [&] {
        EXPECT_FALSE(open);
        open = true;
    });

    std::shared_ptr<lua::api::WebSocket> ws = lua->script(R"lua(
        local ws = c2.WebSocket.new("wss://127.0.0.1:9050/echo", { 
            headers = {
                ["User-Agent"] = "Lua",
                ["A-Header"] = "A value",
                ["Referer"] = "https://chatterino.com",
            },
        })
        ws.on_text = function(data)
            add(true, data)
        end
        local any_msg = false
        ws.on_binary = function(data)
            if not any_msg then
                any_msg = true
                ws:send_text(string.rep("a", 1 << 15))
                ws:send_binary("wow")
                ws:send_text("/HEADER user-agent")
                ws:send_text("/HEADER a-header")
                ws:send_text("/HEADER referer")
                ws:send_binary("/CLOSE")
            end
            add(false, data)
        end
        ws.on_close = function()
            done()
        end
        ws.on_open = function()
            open()
        end
        ws:send_text("message1")
        ws:send_text("message2")
        ws:send_text("message3")
        ws:send_binary("message4")

        return ws
    )lua");
    std::weak_ptr<lua::api::WebSocket> weakWs{ws};
    ws.reset();

    waiter.waitForRequest();
    ASSERT_TRUE(open);

    ASSERT_EQ(messages.size(), 9);
    ASSERT_EQ(messages[0].first, true);
    ASSERT_EQ(messages[0].second, "message1");
    ASSERT_EQ(messages[1].first, true);
    ASSERT_EQ(messages[1].second, "message2");
    ASSERT_EQ(messages[2].first, true);
    ASSERT_EQ(messages[2].second, "message3");
    ASSERT_EQ(messages[3].first, false);
    ASSERT_EQ(messages[3].second, "message4");
    ASSERT_EQ(messages[4].first, true);
    ASSERT_EQ(messages[4].second, QByteArray(1 << 15, 'a'));
    ASSERT_EQ(messages[5].first, false);
    ASSERT_EQ(messages[5].second, "wow");
    ASSERT_EQ(messages[6].first, true);
    ASSERT_EQ(messages[6].second, "Lua");
    ASSERT_EQ(messages[7].first, true);
    ASSERT_EQ(messages[7].second, "A value");
    ASSERT_EQ(messages[8].first, true);
    ASSERT_EQ(messages[8].second, "https://chatterino.com");

    ASSERT_FALSE(weakWs.expired());
    lua->collect_garbage();
    ASSERT_TRUE(weakWs.expired());
}

TEST_F(PluginTest, testWebSocketNoPerms)
{
    configure();

    bool res = lua->script(R"lua(
        return c2["WebSocket"] ~= nil
    )lua");
    ASSERT_TRUE(res);

    const char *shouldThrow = R"lua(
        return c2.WebSocket.new('wss://127.0.0.1:9050/echo')
    )lua";
    EXPECT_ANY_THROW(lua->script(shouldThrow));
}

TEST_F(PluginTest, testWebSocketApi)
{
    configure({PluginPermission{{{"type", "Network"}}}});

    bool ok = lua->script(R"lua(
        local t = function () end
        local b = function () end
        local c = function () end
        local ws = c2.WebSocket.new("wss://127.0.0.1:9050/echo", { 
            on_text = t,
            on_binary = b,
            on_close = c,
        })

        return ws.on_text == t and ws.on_binary == b and ws.on_close == c
    )lua");

    ASSERT_TRUE(ok);
}

TEST_F(PluginTest, testWebSocketUnsetFns)
{
    configure({PluginPermission{{{"type", "Network"}}}});

    RequestWaiter waiter;
    lua->set("done", [&] {
        waiter.requestDone();
    });

    lua->script(R"lua(
        local ws = c2.WebSocket.new("wss://127.0.0.1:9050/echo")
        ws.on_close = function()
            done()
        end
        ws:send_text("message1")
        ws:send_text("message2")
        ws:send_binary("message3")
        ws:send_binary("/CLOSE")
    )lua");

    waiter.waitForRequest();
}

TEST_F(PluginTest, MessageElementFlag)
{
    configure();
    lua->script(R"lua(
        values = {}
        for k, v in pairs(c2.MessageElementFlag) do
            table.insert(values, ("%s=0x%x"):format(k, v))
        end
        table.sort(values, function(a, b) return a:lower() < b:lower() end)
        out = table.concat(values, ",")
    )lua");

    const char *VALUES = "AlwaysShow=0x2000000,"
                         "BadgeBttv=0x40,"
                         "BadgeChannelAuthority=0x8000,"
                         "BadgeChatterino=0x40000,"
                         "BadgeFfz=0x80000,"
                         "BadgeGlobalAuthority=0x2000,"
                         "BadgePredictions=0x4000,"
                         "BadgeSevenTV=0x1000000000,"
                         "BadgeSharedChannel=0x2000000000,"
                         "BadgeSubscription=0x10000,"
                         "BadgeVanity=0x20000,"
                         "BitsAmount=0x200000,"
                         "BitsAnimated=0x1000,"
                         "BitsStatic=0x800,"
                         "ChannelName=0x100000,"
                         "ChannelPointReward=0x100,"
                         "Collapsed=0x4000000,"
                         "EmojiImage=0x800000,"
                         "EmojiText=0x1000000,"
                         "EmoteImage=0x10,"
                         "EmoteText=0x20,"
                         "LowercaseLinks=0x20000000,"
                         "Mention=0x8000000,"
                         "Misc=0x1,"
                         "ModeratorTools=0x400000,"
                         "RepliedMessage=0x100000000,"
                         "ReplyButton=0x200000000,"
                         "Text=0x2,"
                         "Timestamp=0x8,"
                         "Username=0x4";

    std::string got = (*lua)["out"];
    ASSERT_EQ(got, VALUES);
}

TEST_F(PluginTest, ChannelAddMessage)
{
    configure();
    lua->script(R"lua(
        function do_it(chan)
            local Repost = c2.MessageContext.Repost
            local Original = c2.MessageContext.Original
            chan:add_message(c2.Message.new({ id = "1" }))
            chan:add_message(c2.Message.new({ id = "2" }), Repost)
            chan:add_message(c2.Message.new({ id = "3" }), Original, nil)
            chan:add_message(c2.Message.new({ id = "4" }), Repost, c2.MessageFlag.DoNotLog)
            chan:add_message(c2.Message.new({ id = "5" }), Original, c2.MessageFlag.DoNotLog)
            chan:add_message(c2.Message.new({ id = "6" }), Original, c2.MessageFlag.System)
        end
    )lua");

    auto chan = std::make_shared<MockChannel>("mock");

    std::vector<MessagePtr> logged;
    EXPECT_CALL(this->app->logging, addMessage)
        .Times(3)
        .WillRepeatedly(
            [&](const auto &, const auto &msg, const auto &, const auto &) {
                logged.emplace_back(msg);
            });

    std::vector<std::pair<MessagePtr, std::optional<MessageFlags>>> added;
    std::ignore = chan->messageAppended.connect([&](auto &&...args) {
        added.emplace_back(std::forward<decltype(args)>(args)...);
    });

    (*lua)["do_it"](lua::api::ChannelRef(chan));

    ASSERT_EQ(added.size(), 6);
    ASSERT_EQ(added[0].first->id, "1");
    ASSERT_FALSE(added[0].second.has_value());
    ASSERT_EQ(added[1].first->id, "2");
    ASSERT_FALSE(added[1].second.has_value());
    ASSERT_EQ(added[2].first->id, "3");
    ASSERT_FALSE(added[2].second.has_value());
    ASSERT_EQ(added[3].first->id, "4");
    ASSERT_EQ(added[3].second, MessageFlags{MessageFlag::DoNotLog});
    ASSERT_EQ(added[4].first->id, "5");
    ASSERT_EQ(added[4].second, MessageFlags{MessageFlag::DoNotLog});
    ASSERT_EQ(added[5].first->id, "6");
    ASSERT_EQ(added[5].second, MessageFlags{MessageFlag::System});

    ASSERT_EQ(logged.size(), 3);
    ASSERT_EQ(added[0].first, logged[0]);
    ASSERT_EQ(added[2].first, logged[1]);
    ASSERT_EQ(added[5].first, logged[2]);
}

TEST_F(PluginTest, MessageFrozenFlag)
{
    configure();
    sol::protected_function isFrozenFn = lua->script(R"lua(
        return function(msg)
            return msg.frozen
        end
    )lua");
    sol::protected_function setFrozenFn = lua->script(R"lua(
        return function(msg, val)
            msg.frozen = val
        end
    )lua");

    auto liquid = std::make_shared<Message>();
    auto res = isFrozenFn(liquid);
    ASSERT_TRUE(res.valid());
    ASSERT_FALSE(res.get<bool>());

    auto frozen = std::make_shared<Message>();
    frozen->freeze();
    res = isFrozenFn(frozen);
    ASSERT_TRUE(res.valid());
    ASSERT_TRUE(res.get<bool>());

    // we shouldn't be able to modify the flag
    ASSERT_FALSE(setFrozenFn(liquid, true).valid());
    ASSERT_FALSE(setFrozenFn(liquid, false).valid());
    ASSERT_FALSE(setFrozenFn(frozen, true).valid());
    ASSERT_FALSE(setFrozenFn(frozen, false).valid());
}

TEST_F(PluginTest, MessageFlagModification)
{
    configure();
    sol::protected_function pfn = lua->script(R"lua(
        return function(msg)
            assert(msg.flags == c2.MessageFlag.Debug)
            msg.flags = c2.MessageFlag.System
            assert(msg.flags == c2.MessageFlag.System)
        end
    )lua");
    sol::protected_function isFrozenFn = lua->script(R"lua(
        return function(msg)
            return msg.frozen
        end
    )lua");

    auto liquid = std::make_shared<Message>();
    liquid->flags = MessageFlag::Debug;
    auto res = pfn(liquid);
    ASSERT_TRUE(res.valid());

    // for the flags, it shouldn't matter if the message is frozen
    auto frozen = std::make_shared<Message>();
    frozen->flags = MessageFlag::Debug;
    frozen->freeze();
    res = pfn(frozen);
    ASSERT_TRUE(res.valid());
}

TEST_F(PluginTest, MessageModification)
{
    configure();

    // Test that we can modify properties and that Lua sees the modification
    sol::table tests = lua->script(R"lua(
        return {
            function(msg)
                msg.parse_time = 1234567
            end,
            function(msg)
                assert(msg.id == "abc")
                msg.id = "1234"
                assert(msg.id == "1234")
            end,
            function(msg)
                assert(msg.search_text == "search")
                msg.search_text = "query"
                assert(msg.search_text == "query")
            end,
            function(msg)
                assert(msg.message_text == "msg")
                msg.message_text = "text"
                assert(msg.message_text == "text")
            end,
            function(msg)
                assert(msg.login_name == "login")
                msg.login_name = "name"
                assert(msg.login_name == "name")
            end,
            function(msg)
                assert(msg.display_name == "display")
                msg.display_name = "name"
                assert(msg.display_name == "name")
            end,
            function(msg)
                assert(msg.localized_name == "localized")
                msg.localized_name = "name"
                assert(msg.localized_name == "name")
            end,
            function(msg)
                assert(msg.user_id == "id")
                msg.user_id = "id"
                assert(msg.user_id == "id")
            end,
            function(msg)
                assert(msg.channel_name == "channel")
                msg.channel_name = "name"
                assert(msg.channel_name == "name")
            end,
            function(msg)
                assert(msg.username_color == "#ffaabbcc")
                msg.username_color = "#ccbbaaff"
                assert(msg.username_color == "#ccbbaaff")
            end,
            function(msg)
                assert(msg.server_received_time == 1230000)
                msg.server_received_time = 1240000
                assert(msg.server_received_time == 1240000)
            end,
            function(msg)
                print(msg.highlight_color)
                assert(msg.highlight_color == "#ff223344")
                msg.highlight_color = "#44332211"
                assert(msg.highlight_color == "#44332211")
            end,
            function(msg)
                assert(#msg:elements() == 2)
                msg:append_element({ type = "linebreak" })
                assert(#msg:elements() == 3)
                assert(msg:elements()[3].type == "linebreak")
            end,
        }
    )lua");

    auto makeMsg = [] {
        auto msg = std::make_shared<Message>();
        msg->flags = MessageFlag::Debug;
        msg->id = "abc";
        msg->searchText = "search";
        msg->messageText = "msg";
        msg->loginName = "login";
        msg->displayName = "display";
        msg->localizedName = "localized";
        msg->userID = "id";
        msg->channelName = "channel";
        msg->usernameColor = QColor(0xaabbcc);
        msg->serverReceivedTime = QDateTime::fromMSecsSinceEpoch(1230000);
        msg->highlightColor = std::make_shared<QColor>(0x223344);
        msg->elements.push_back(
            std::make_unique<TextElement>("lol", MessageElementFlag::Text));
        msg->elements.push_back(
            std::make_unique<TextElement>("wow", MessageElementFlag::Text));
        return msg;
    };

    auto liquid = makeMsg();
    ASSERT_TRUE(tests.valid());
    for (const auto &[_key, cb] : tests)
    {
        sol::protected_function pf = cb;
        auto res = pf(liquid);
        if (!res.valid())
        {
            sol::error err = res;
            ASSERT_TRUE(false) << err.what();
        }
    }

    // If the message is frozen, all modifications should fail with an error
    auto frozen = makeMsg();
    frozen->freeze();
    for (const auto &[key, cb] : tests)
    {
        sol::protected_function pf = cb;
        auto res = pf(frozen);
        ASSERT_FALSE(res.valid());
        sol::error err = res;
        ASSERT_EQ(std::string_view(err.what()), "Message is frozen");
    }
}

TEST_F(PluginTest, MessageConstness)
{
    configure();
    sol::protected_function pfn = lua->script(R"lua(
        return function(msg)
            assert(msg.login_name == "hello")
            msg.login_name = "alien"
        end
    )lua");

    auto msg = std::make_shared<Message>();
    msg->loginName = "hello";
    MessagePtr cmsg = msg;

    auto res = pfn(cmsg);
    ASSERT_TRUE(res.valid());
    cmsg->freeze();
    res = pfn(cmsg);
    ASSERT_FALSE(res.valid());
}

// Test that we can access properties of message elements
TEST_F(PluginTest, MessageElementAccess)
{
    configure();
    sol::protected_function pfn = lua->script(R"lua(
        return function(msg, idx, prop)
            return msg:elements()[idx][prop]
        end
    )lua");

    auto msg = std::make_shared<Message>();
    msg->elements.emplace_back(
        std::make_unique<TextElement>("my text", MessageElementFlag::Text));
    msg->elements.emplace_back(std::make_unique<SingleLineTextElement>(
        "single line", MessageElementFlag::Text));
    msg->elements.emplace_back(std::make_unique<CircularImageElement>(
        ImagePtr{}, 2, QColor(0xabcdef), MessageElementFlag::ReplyButton));
    msg->elements.emplace_back(std::make_unique<MentionElement>(
        "display", "login", MessageColor::Text, MessageColor::System));

    msg->elements[1]->setTooltip("tooltip");
    msg->elements[2]->setTrailingSpace(false);
    msg->freeze();

    auto getAll = [&](std::string_view key) {
        return std::array{
            pfn(msg, 1, key).get<sol::object>(),
            pfn(msg, 2, key).get<sol::object>(),
            pfn(msg, 3, key).get<sol::object>(),
            pfn(msg, 4, key).get<sol::object>(),
        };
    };

    auto types = getAll("type");
    ASSERT_EQ(types[0].as<std::string>(), "text");
    ASSERT_EQ(types[1].as<std::string>(), "single-line-text");
    ASSERT_EQ(types[2].as<std::string>(), "circular-image");
    ASSERT_EQ(types[3].as<std::string>(), "mention");

    auto flags = getAll("flags");
    ASSERT_EQ(flags[0].as<MessageElementFlag>(), MessageElementFlag::Text);
    ASSERT_EQ(flags[1].as<MessageElementFlag>(), MessageElementFlag::Text);
    ASSERT_EQ(flags[2].as<MessageElementFlag>(),
              MessageElementFlag::ReplyButton);
    ASSERT_EQ(flags[3].as<MessageElementFlag>(),
              (MessageElementFlags(MessageElementFlag::Text,
                                   MessageElementFlag::Mention)));

    auto tooltips = getAll("tooltip");
    ASSERT_EQ(tooltips[0].as<std::string>(), "");
    ASSERT_EQ(tooltips[1].as<std::string>(), "tooltip");
    ASSERT_EQ(tooltips[2].as<std::string>(), "");
    ASSERT_EQ(tooltips[3].as<std::string>(), "");

    auto spaces = getAll("trailing_space");
    ASSERT_TRUE(spaces[0].as<bool>());
    ASSERT_TRUE(spaces[1].as<bool>());
    ASSERT_FALSE(spaces[2].as<bool>());
    ASSERT_TRUE(spaces[3].as<bool>());

    // Properties only found on _some_ elements should not error
    // (like non existent properties)
    auto paddings = getAll("padding");
    ASSERT_TRUE(paddings[0].is<std::nullptr_t>());
    ASSERT_TRUE(paddings[1].is<std::nullptr_t>());
    ASSERT_EQ(paddings[2].as<int>(), 2);
    ASSERT_TRUE(paddings[3].is<std::nullptr_t>());

    auto words = getAll("words");
    ASSERT_EQ(words[0].as<std::vector<std::string>>(),
              (std::vector<std::string>{"my", "text"}));
    ASSERT_EQ(words[1].as<std::vector<std::string>>(),
              (std::vector<std::string>{"single", "line"}));
    ASSERT_TRUE(words[2].is<std::nullptr_t>());
    // mention elements are also text elements
    ASSERT_EQ(words[3].as<std::vector<std::string>>(),
              (std::vector<std::string>{"display"}));

    auto userLogins = getAll("user_login_name");
    ASSERT_TRUE(userLogins[0].is<std::nullptr_t>());
    ASSERT_TRUE(userLogins[1].is<std::nullptr_t>());
    ASSERT_TRUE(userLogins[2].is<std::nullptr_t>());
    ASSERT_EQ(userLogins[3].as<std::string>(), "login");

    auto times = getAll("time");
    ASSERT_TRUE(times[0].is<std::nullptr_t>());
    ASSERT_TRUE(times[1].is<std::nullptr_t>());
    ASSERT_TRUE(times[2].is<std::nullptr_t>());
    ASSERT_TRUE(times[3].is<std::nullptr_t>());

    auto nonExistent = getAll("non_existent");
    ASSERT_TRUE(nonExistent[0].is<std::nullptr_t>());
    ASSERT_TRUE(nonExistent[1].is<std::nullptr_t>());
    ASSERT_TRUE(nonExistent[2].is<std::nullptr_t>());
    ASSERT_TRUE(nonExistent[3].is<std::nullptr_t>());

    auto links = getAll("link");
    ASSERT_TRUE(links[0].is<Link>());
    ASSERT_TRUE(links[1].is<Link>());
    ASSERT_TRUE(links[2].is<Link>());
    ASSERT_TRUE(links[3].is<Link>());

    // test that accessing anything outside the elements vector causes an error
    auto res = pfn(msg, 0, "flags");
    ASSERT_FALSE(res.valid());
    res = pfn(msg, 42, "flags");
    ASSERT_FALSE(res.valid());
}

// Test that we can modify properties of message elements
TEST_F(PluginTest, MessageElementModification)
{
    configure();
    sol::protected_function pfn = lua->script(R"lua(
        return function(msg, idx, prop, val)
            msg:elements()[idx][prop] = val
        end
    )lua");

    // same as MessageElementAccess...
    auto msg = std::make_shared<Message>();
    msg->elements.emplace_back(
        std::make_unique<TextElement>("my text", MessageElementFlag::Text));
    msg->elements.emplace_back(std::make_unique<SingleLineTextElement>(
        "single line", MessageElementFlag::Text));
    msg->elements.emplace_back(std::make_unique<CircularImageElement>(
        ImagePtr{}, 2, QColor(0xabcdef), MessageElementFlag::ReplyButton));
    msg->elements.emplace_back(std::make_unique<MentionElement>(
        "display", "login", MessageColor::Text, MessageColor::System));

    msg->elements[1]->setTooltip("tooltip");
    msg->elements[2]->setTrailingSpace(false);
    // ...but we don't freeze the message here

    auto setAll = [&](std::string_view key, auto value) {
        for (size_t i = 1; i <= 4; i++)
        {
            EXPECT_TRUE(pfn(msg, i, key, value).valid());
        }
    };
    setAll("tooltip", "tool");
    ASSERT_EQ(msg->elements[0]->getTooltip(), "tool");
    ASSERT_EQ(msg->elements[1]->getTooltip(), "tool");
    ASSERT_EQ(msg->elements[2]->getTooltip(), "tool");
    ASSERT_EQ(msg->elements[3]->getTooltip(), "tool");

    setAll("trailing_space", false);
    ASSERT_FALSE(msg->elements[0]->hasTrailingSpace());
    ASSERT_FALSE(msg->elements[1]->hasTrailingSpace());
    ASSERT_FALSE(msg->elements[2]->hasTrailingSpace());
    ASSERT_FALSE(msg->elements[3]->hasTrailingSpace());

    pfn(msg, 1, "link", Link{Link::CopyToClipboard, "foo"});
    pfn(msg, 2, "link", Link{Link::CopyToClipboard, "foo"});
    pfn(msg, 3, "link", Link{Link::CopyToClipboard, "foo"});
    // can't modify links of mention elements
    ASSERT_FALSE(
        pfn(msg, 4, "link", Link{Link::CopyToClipboard, "foo"}).valid());

    ASSERT_EQ(msg->elements[0]->getLink().type, Link::CopyToClipboard);
    ASSERT_EQ(msg->elements[0]->getLink().value, "foo");
    ASSERT_EQ(msg->elements[0]->getTooltip(), "<b>Copy to clipboard</b>");
    ASSERT_EQ(msg->elements[1]->getLink().type, Link::CopyToClipboard);
    ASSERT_EQ(msg->elements[1]->getLink().value, "foo");
    ASSERT_EQ(msg->elements[2]->getLink().type, Link::CopyToClipboard);
    ASSERT_EQ(msg->elements[2]->getLink().value, "foo");

    auto expectErr = [&](std::string_view key, auto value) {
        for (size_t i = 1; i <= 4; i++)
        {
            auto result = pfn(msg, i, key, value);
            EXPECT_FALSE(result.valid()) << key;
        }
    };
    expectErr("type", "something");
    expectErr("trailing_space", "something");

    // can't set these types
    expectErr("link", Link{Link::ViewThread, "foo"});
    expectErr("link", Link{Link::AutoModAllow, "foo"});

    // We can't modify these yet
    expectErr("padding", 1);
    expectErr("background", 0xabcdef12);
    expectErr("words", QStringList{"a", "b"});
    expectErr("color", 0x1234);
    expectErr("style", FontStyle::ChatMedium);
    expectErr("lowercase", "abc");
    expectErr("original", "or");
    expectErr("fallback_color", "system");
    expectErr("user_color", "system");
    expectErr("user_login_name", "system");
    expectErr("time", 42);

    // test that accessing anything outside the elements vector causes an error
    auto res = pfn(msg, 0, "trailing_space", true);
    ASSERT_FALSE(res.valid());
    res = pfn(msg, 42, "trailing_space", true);
    ASSERT_FALSE(res.valid());

    // we can't modify anything on frozen messages
    msg->freeze();
    expectErr("tooltip", "tool");
    expectErr("trailing_space", false);
    expectErr("padding", 1);
}

/// Test that both C++ exceptions and luaL_error properly unwind the stack.
TEST_F(PluginTest, LuaUnwind)
{
    configure();

    size_t i = 0;
    lua->set_function(
        "do_something",
        [&](sol::this_state state, bool should_error, bool use_lua_error) {
            auto g = qScopeGuard([&] {
                ++i;
            });
            if (should_error)
            {
                if (use_lua_error)
                {
                    luaL_error(state.lua_state(), "My message");
                }
                else
                {
                    throw std::runtime_error("My message");
                }
            }
        });

    ASSERT_EQ(i, 0);

    ASSERT_TRUE(lua->do_string("do_something(false, false)").valid());
    ASSERT_EQ(i, 1);

    ASSERT_TRUE(lua->do_string("do_something(false, true)").valid());
    ASSERT_EQ(i, 2);

    ASSERT_FALSE(lua->do_string("do_something(true, false)").valid());
    ASSERT_EQ(i, 3);

    ASSERT_FALSE(lua->do_string("do_something(true, true)").valid());
    ASSERT_EQ(i, 4);
}

/// Test that we're running with the Lua version we're compiled against.
TEST_F(PluginTest, LuaVersion)
{
    configure();

    lua->set_function("check_it", [](sol::this_state state) {
        luaL_checkversion(state.lua_state());
    });
    ASSERT_TRUE(lua->script("check_it()").valid());

    static_assert(LUA_VERSION_NUM >= 504);
}

class PluginMessageConstructionTest
    : public PluginTest,
      public ::testing::WithParamInterface<QString>
{
};
TEST_P(PluginMessageConstructionTest, Run)
{
    auto fixture = testlib::Snapshot::read("PluginMessageCtor", GetParam());

    configure();
    std::string script;
    if (fixture->input().isArray())
    {
        for (auto line : fixture->input().toArray())
        {
            script += line.toString().toStdString() + '\n';
        }
    }
    else
    {
        script = fixture->inputString().toStdString() + '\n';
    }

    script += "out = c2.Message.new(msg)";
    lua->script(script);

    Message *got = (*lua)["out"];

    ASSERT_TRUE(fixture->run(got->toJson(), UPDATE_SNAPSHOTS));
}

INSTANTIATE_TEST_SUITE_P(
    PluginMessageConstruction, PluginMessageConstructionTest,
    testing::ValuesIn(testlib::Snapshot::discover("PluginMessageCtor")));

class PluginJsonTest : public PluginTest,
                       public ::testing::WithParamInterface<QString>
{
};
TEST_P(PluginJsonTest, Run)
{
    configure();
    auto reg = lua->registry().size();
    auto globals = lua->globals().size();

    runLuaTest("json", GetParam(), *this->lua);

    for (size_t i = 0; i < 5; i++)
    {
        lua->collect_garbage();
    }
    // make sure we don't leak anything to globals or the registry
    // but give the registry some room of 3 slots (two from getting the debug library)
    EXPECT_LE(lua->registry().size(), reg + 3);
    EXPECT_EQ(lua->globals().size(), globals);
}

INSTANTIATE_TEST_SUITE_P(PluginJson, PluginJsonTest,
                         testing::ValuesIn(discoverLuaTests("json")));

class PluginMessageTest : public PluginTest,
                          public ::testing::WithParamInterface<QString>
{
};
TEST_P(PluginMessageTest, Run)
{
    this->configure();
    runLuaTest("message", GetParam(), *this->lua);
}

INSTANTIATE_TEST_SUITE_P(PluginMessage, PluginMessageTest,
                         testing::ValuesIn(discoverLuaTests("message")));

// verify that all snapshots are included
TEST(PluginMessageConstructionTest, Integrity)
{
    ASSERT_FALSE(UPDATE_SNAPSHOTS);  // make sure fixtures are actually tested
}

TEST_F(PluginTest, testAccounts)
{
    configure();

    auto res = lua->script(R"lua(
        local current = c2.current_account()
        assert(current:login() == "testaccount_420")
        assert(current:id() == "117166826")
        assert(current:color() == nil) -- unset
        assert(not current:is_anon())
    )lua");
    ASSERT_TRUE(res.valid()) << res.get<sol::error>().what();
}

TEST_F(PluginTest, debugTraceback)
{
    configure();

    QString traceback = lua->script(R"lua(
        local function other()
            error("oh no")
        end
        local function main()
            local function inner()
                other()
            end
            inner()
        end

        local ok, res = xpcall(main, debug.traceback)
        return res
    )lua")
                            .get<QString>();
    ASSERT_TRUE(traceback.contains("[C]: in function 'error'"));
    ASSERT_TRUE(traceback.contains("[string \"...\"]:3: in upvalue 'other'"));
    ASSERT_TRUE(traceback.contains("[string \"...\"]:7: in local 'inner'"));
    ASSERT_TRUE(traceback.contains(
        "[string \"...\"]:9: in function <[string \"...\"]:5>"));
    ASSERT_TRUE(traceback.contains("[C]: in function 'xpcall'"));
    ASSERT_TRUE(traceback.contains("[string \"...\"]:12: in main chunk"));

    traceback = lua->script(R"lua(
        return debug.traceback()
    )lua")
                    .get<QString>();
    ASSERT_TRUE(traceback.contains("[string \"...\"]:2: in main chunk"));

    traceback = lua->script(R"lua(
        return debug.traceback("my message")
    )lua")
                    .get<QString>();
    ASSERT_TRUE(traceback.contains("my message"));
    ASSERT_TRUE(traceback.contains("[string \"...\"]:2: in main chunk"));

    traceback = lua->script(R"lua(
        local coro = coroutine.create(function ()
            coroutine.yield()
        end)
        coroutine.resume(coro)
        return debug.traceback(coro)
    )lua")
                    .get<QString>();
    ASSERT_TRUE(traceback.contains("[C]: in function 'coroutine.yield'"));
    ASSERT_TRUE(traceback.contains(
        "[string \"...\"]:3: in function <[string \"...\"]:2>"));

    auto msg = lua->script(R"lua(
        return debug.traceback(c2.Message.new({id = "who would do this"}))
    )lua")
                   .get<std::shared_ptr<Message>>();
    ASSERT_EQ(msg->id, "who would do this");
}

#endif
