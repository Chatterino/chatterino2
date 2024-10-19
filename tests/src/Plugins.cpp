#ifdef CHATTERINO_HAVE_PLUGINS
#    include "Application.hpp"
#    include "common/Channel.hpp"
#    include "common/network/NetworkCommon.hpp"
#    include "controllers/commands/Command.hpp"  // IWYU pragma: keep
#    include "controllers/commands/CommandController.hpp"
#    include "controllers/plugins/api/ChannelRef.hpp"
#    include "controllers/plugins/Plugin.hpp"
#    include "controllers/plugins/PluginController.hpp"
#    include "controllers/plugins/PluginPermission.hpp"
#    include "controllers/plugins/SolTypes.hpp"  // IWYU pragma: keep
#    include "mocks/BaseApplication.hpp"
#    include "mocks/Channel.hpp"
#    include "mocks/Emotes.hpp"
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

const QString TEST_SETTINGS = R"(
{
    "plugins": {
        "supportEnabled": true,
        "enabledPlugins": [
            "test"
        ]
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

    IEmotes *getEmotes() override
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

    PluginController plugins;
    mock::EmptyLogging logging;
    CommandController commands;
    mock::Emotes emotes;
    MockTwitch twitch;
};

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
        return PluginController::openLibrariesFor(plugin);
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
    f.open(QFile::WriteOnly);
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
    f.open(QFile::WriteOnly);
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

#endif
