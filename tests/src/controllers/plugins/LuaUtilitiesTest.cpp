// SPDX-FileCopyrightText: 2026 Contributors to Chatterino <https://chatterino.com>
//
// SPDX-License-Identifier: MIT

#include "controllers/plugins/LuaMethodRef.hpp"
#include "controllers/plugins/PluginTestApp.hpp"
#include "controllers/plugins/SignalCallback.hpp"
#include "controllers/plugins/SolTypes.hpp"
#include "Test.hpp"

namespace {

struct MyQObject : public QObject {
    Q_OBJECT

Q_SIGNALS:
    void empty();
    void boolArg(bool b);                     // NOLINT(readability-*)
    void multiArg(const QString &qs, int i);  // NOLINT(readability-*)
};

}  // namespace

namespace chatterino::lua {

// MARK: LuaFunctionRef

TEST(LuaUtilitiesFunctionRef, call)
{
    testlib::PluginApplication app;

    auto *plugin1 = app.loadPlugin("plugin1");
    auto weak = plugin1->weakRef();
    std::optional state = plugin1->state();

    std::string lastCall;
    state->set_function("was_called", [&](std::string fn) {
        lastCall = std::move(fn);
    });
    state->safe_script(R"(
        function empty(...)
            assert(#{...} == 0)
            was_called("empty")
        end
        function bool(b, ...)
            assert(b == true and #{...} == 0)
            was_called("bool")
        end
        function multi(s, i, ...)
            assert(s == "foo" and i == 42)
            was_called("multi")
            return i
        end
    )");

    LuaFunctionRef fnEmpty(weak, (*state)["empty"]);
    LuaFunctionRef fnBool(weak, (*state)["bool"]);
    LuaFunctionRef fnMulti(weak, (*state)["multi"]);

    ASSERT_TRUE(fnEmpty.isAlive());
    ASSERT_TRUE(fnBool.isAlive());
    ASSERT_TRUE(fnMulti.isAlive());

    ASSERT_EQ(fnEmpty.owner(), weak);
    ASSERT_EQ(fnBool.owner(), weak);
    ASSERT_EQ(fnMulti.owner(), weak);

    ASSERT_EQ(lastCall, "");
    ASSERT_TRUE(fnEmpty.tryCall<void>(u"ctx"));
    ASSERT_EQ(lastCall, "empty");
    ASSERT_TRUE(fnBool.tryCall<void>(u"ctx", true));
    ASSERT_EQ(lastCall, "bool");
    ASSERT_EQ(fnMulti.tryCall<int>(u"ctx", "foo", 42), 42);
    ASSERT_EQ(lastCall, "multi");

    // Wrong return value.
    ASSERT_EQ(fnEmpty.tryCall<int>(u"ctx"), std::nullopt);
    ASSERT_EQ(lastCall, "empty");

    state.reset();
    app.removePlugin("plugin1");

    ASSERT_FALSE(fnEmpty.isAlive());
    ASSERT_FALSE(fnBool.isAlive());
    ASSERT_FALSE(fnMulti.isAlive());

    lastCall = "";
    ASSERT_FALSE(fnEmpty.tryCall<void>(u"ctx"));
    ASSERT_EQ(lastCall, "");
    ASSERT_FALSE(fnBool.tryCall<void>(u"ctx", true));
    ASSERT_EQ(lastCall, "");
    ASSERT_EQ(fnMulti.tryCall<int>(u"ctx", "foo", 42), std::nullopt);
    ASSERT_EQ(lastCall, "");
}

TEST(LuaUtilitiesFunctionRef, specialMembers)
{
    testlib::PluginApplication app;

    auto *plugin1 = app.loadPlugin("plugin1");
    auto weak = plugin1->weakRef();
    std::optional state = plugin1->state();

    uint32_t called = 0;
    state->set_function("called", [&] {
        ++called;
    });
    state->safe_script("function foo() called() end");

    LuaFunctionRef fnDefault;
    ASSERT_FALSE(fnDefault.isAlive());
    ASSERT_NE(fnDefault.owner(), weak);
    ASSERT_EQ(fnDefault.owner(), PluginWeakRef{});
    ASSERT_FALSE(fnDefault.tryCall<void>({}));
    ASSERT_FALSE(fnDefault.tryCall<void>({}, 123, true, false));
    ASSERT_EQ(fnDefault.tryCall<int>({}), std::nullopt);
    fnDefault.reset();
    ASSERT_FALSE(fnDefault.isAlive());
    ASSERT_EQ(fnDefault.owner(), PluginWeakRef{});
    ASSERT_FALSE(fnDefault.tryCall<void>({}));

    LuaFunctionRef fnFoo(weak, (*state)["foo"]);
    ASSERT_TRUE(fnFoo.isAlive());
    ASSERT_EQ(fnFoo.owner(), weak);

    ASSERT_EQ(called, 0);
    ASSERT_TRUE(fnFoo.tryCall<void>({}));
    ASSERT_EQ(called, 1);
    ASSERT_TRUE(fnFoo.tryCall<void>({}, 123));
    ASSERT_EQ(called, 2);

    LuaFunctionRef fnFooCopy = fnFoo;
    ASSERT_TRUE(fnFoo.isAlive());
    ASSERT_TRUE(fnFooCopy.isAlive());
    ASSERT_EQ(called, 2);
    ASSERT_TRUE(fnFooCopy.tryCall<void>({}));
    ASSERT_EQ(called, 3);

    LuaFunctionRef fnFooMoved(std::move(fnFoo));
    ASSERT_FALSE(fnFoo.isAlive());
    ASSERT_TRUE(fnFooCopy.isAlive());
    ASSERT_TRUE(fnFooMoved.isAlive());
    ASSERT_EQ(fnFooMoved.owner(), weak);
    ASSERT_NE(fnFoo.owner(), weak);
    ASSERT_EQ(fnFoo.owner(), PluginWeakRef{});

    ASSERT_EQ(called, 3);
    ASSERT_FALSE(fnFoo.tryCall<void>({}));
    ASSERT_EQ(called, 3);
    ASSERT_TRUE(fnFooCopy.tryCall<void>({}));
    ASSERT_EQ(called, 4);
    ASSERT_TRUE(fnFooMoved.tryCall<void>({}));
    ASSERT_EQ(called, 5);

    LuaFunctionRef fnFooMoveAssigned;
    ASSERT_FALSE(fnFooMoveAssigned.isAlive());
    fnFooMoveAssigned = std::move(fnFooMoved);

    ASSERT_FALSE(fnFooMoved.isAlive());
    ASSERT_TRUE(fnFooMoveAssigned.isAlive());

    ASSERT_EQ(called, 5);
    ASSERT_FALSE(fnFooMoved.tryCall<void>({}));
    ASSERT_EQ(called, 5);
    ASSERT_TRUE(fnFooMoveAssigned.tryCall<void>({}));
    ASSERT_EQ(called, 6);

    LuaFunctionRef fnFooCopyAssigned;
    ASSERT_FALSE(fnFooCopyAssigned.isAlive());
    fnFooCopyAssigned = fnFooMoveAssigned;
    ASSERT_TRUE(fnFooCopyAssigned.isAlive());
    ASSERT_TRUE(fnFooMoveAssigned.isAlive());

    ASSERT_EQ(called, 6);
    ASSERT_TRUE(fnFooMoveAssigned.tryCall<void>({}));
    ASSERT_EQ(called, 7);
    ASSERT_TRUE(fnFooCopyAssigned.tryCall<void>({}));
    ASSERT_EQ(called, 8);

    fnFooCopyAssigned.reset();
    ASSERT_FALSE(fnFooCopyAssigned.isAlive());
    ASSERT_NE(fnFooCopyAssigned.owner(), weak);
    ASSERT_EQ(fnFooCopyAssigned.owner(), PluginWeakRef{});
    ASSERT_FALSE(fnFooCopyAssigned.tryCall<void>({}));
    ASSERT_EQ(called, 8);
}

// MARK: SignalCallback

TEST(LuaUtilitiesSignalCallback, qObjectCompat)
{
    testlib::PluginApplication app;

    auto *plugin1 = app.loadPlugin("plugin1");
    auto weak = plugin1->weakRef();
    std::optional state = plugin1->state();

    std::string lastCall;
    state->set_function("was_called", [&](std::string fn) {
        lastCall = std::move(fn);
    });
    state->safe_script(R"(
        function take_empty(...)
            assert(#{...} == 0)
            was_called("empty")
        end
        function take_bool(b, ...)
            assert(b == true and #{...} == 0)
            was_called("bool")
        end
        function take_multi(qs, i, ...)
            assert(qs == "foo" and i == 42)
            was_called("multi")
        end
    )");

    MyQObject obj;
    QObject::connect(&obj, &MyQObject::empty,
                     SignalCallback(weak, (*state)["take_empty"]));
    QObject::connect(&obj, &MyQObject::boolArg,
                     SignalCallback(weak, (*state)["take_bool"]));
    QObject::connect(&obj, &MyQObject::multiArg,
                     SignalCallback(weak, (*state)["take_multi"]));

    ASSERT_EQ(lastCall, "");
    obj.empty();
    ASSERT_EQ(lastCall, "empty");
    obj.boolArg(true);
    ASSERT_EQ(lastCall, "bool");
    obj.multiArg("foo", 42);
    ASSERT_EQ(lastCall, "multi");
    lastCall = "";

    // state_view keeps a reference, so destroy it before the plugin.
    state.reset();
    app.removePlugin("plugin1");

    ASSERT_EQ(lastCall, "");
    obj.empty();
    ASSERT_EQ(lastCall, "");
    obj.boolArg(true);
    ASSERT_EQ(lastCall, "");
    obj.multiArg("foo", 42);
    ASSERT_EQ(lastCall, "");
}

TEST(LuaUtilitiesSignalCallback, pajladaSignals)
{
    testlib::PluginApplication app;

    auto *plugin1 = app.loadPlugin("plugin1");
    auto weak = plugin1->weakRef();
    std::optional state = plugin1->state();

    std::string lastCall;
    state->set_function("was_called", [&](std::string fn) {
        lastCall = std::move(fn);
    });
    state->safe_script(R"(
        function take_empty(...)
            assert(#{...} == 0)
            was_called("empty")
        end
        function take_bool(b, ...)
            assert(b == true and #{...} == 0)
            was_called("bool")
        end
        function take_multi(qs, i, ...)
            assert(qs == "foo" and i == 42)
            was_called("multi")
        end
    )");

    pajlada::Signals::Signal<> empty;
    pajlada::Signals::Signal<bool> boolSignal;
    pajlada::Signals::Signal<const QString &, int> multiSignal;

    std::ignore = empty.connect(SignalCallback(weak, (*state)["take_empty"]));
    std::ignore =
        boolSignal.connect(SignalCallback(weak, (*state)["take_bool"]));
    std::ignore =
        multiSignal.connect(SignalCallback(weak, (*state)["take_multi"]));

    ASSERT_EQ(lastCall, "");
    empty.invoke();
    ASSERT_EQ(lastCall, "empty");
    boolSignal.invoke(true);
    ASSERT_EQ(lastCall, "bool");
    multiSignal.invoke("foo", 42);
    ASSERT_EQ(lastCall, "multi");
    lastCall = "";

    // state_view keeps a reference, so destroy it before the plugin.
    state.reset();
    app.removePlugin("plugin1");

    ASSERT_EQ(lastCall, "");
    empty.invoke();
    ASSERT_EQ(lastCall, "");
    boolSignal.invoke(true);
    ASSERT_EQ(lastCall, "");
    multiSignal.invoke("foo", 42);
    ASSERT_EQ(lastCall, "");
}

// MARK: LuaMethodRef

TEST(LuaUtilitiesMethodRef, tryCall)
{
    testlib::PluginApplication app;

    auto *plugin1 = app.loadPlugin("plugin1");
    auto weak = plugin1->weakRef();
    std::optional state = plugin1->state();

    std::string lastCall;
    state->set_function("was_called", [&](std::string fn) {
        lastCall = std::move(fn);
    });
    sol::table myObj = state->safe_script(R"(
        local my_obj = {}
        function my_obj:empty(...)
            assert(self == my_obj and #{...} == 0)
            was_called("empty")
        end
        function my_obj:one(arg, ...)
            assert(self == my_obj and arg == 42 and #{...} == 0)
            was_called("one")
            return arg + 1
        end
        my_obj.inner = function(a, ...)
            assert(a == my_obj and #{...} == 0)
            was_called("inner")
        end
        return my_obj
    )");

    LuaMethodRef mrEmpty(weak, myObj["empty"], myObj);
    LuaMethodRef mrOne(weak, myObj["one"], myObj);
    LuaMethodRef mrInner(weak, myObj["inner"], myObj);

    ASSERT_TRUE(mrEmpty.isAlive());
    ASSERT_TRUE(mrOne.isAlive());
    ASSERT_TRUE(mrInner.isAlive());

    ASSERT_EQ(lastCall, "");
    ASSERT_TRUE(mrEmpty.tryCall<void>(u"ctx"));
    ASSERT_EQ(lastCall, "empty");
    ASSERT_EQ(mrOne.tryCall<int>(u"ctx", 42), 43);
    ASSERT_EQ(lastCall, "one");
    ASSERT_TRUE(mrInner.tryCall<void>(u"ctx"));
    ASSERT_EQ(lastCall, "inner");
    lastCall = "";
    mrInner.reset();
    ASSERT_FALSE(mrInner.isAlive());
    ASSERT_FALSE(mrInner.tryCall<void>(u"ctx"));

    // state_view keeps a reference, so destroy it before the plugin.
    state.reset();
    myObj.reset();
    app.removePlugin("plugin1");

    ASSERT_FALSE(mrEmpty.isAlive());
    ASSERT_FALSE(mrOne.isAlive());
    ASSERT_FALSE(mrInner.isAlive());

    ASSERT_EQ(lastCall, "");
    ASSERT_FALSE(mrEmpty.tryCall<void>(u"ctx"));
    ASSERT_EQ(lastCall, "");
    ASSERT_EQ(mrOne.tryCall<int>(u"ctx", 43), std::nullopt);
    ASSERT_EQ(lastCall, "");
    ASSERT_FALSE(mrInner.tryCall<int>(u"ctx"));
    ASSERT_EQ(lastCall, "");
}

TEST(LuaUtilitiesMethodRef, specialMembers)
{
    testlib::PluginApplication app;

    auto *plugin1 = app.loadPlugin("plugin1");
    auto weak = plugin1->weakRef();
    std::optional state = plugin1->state();

    uint32_t called = 0;
    state->set_function("called", [&] {
        ++called;
    });
    state->safe_script(R"(
        tbl = {}
        function tbl:foo()
            assert(self == tbl)
            called()
        end
    )");

    LuaMethodRef fnDefault;
    ASSERT_FALSE(fnDefault.isAlive());
    ASSERT_NE(fnDefault.owner(), weak);
    ASSERT_EQ(fnDefault.owner(), PluginWeakRef{});
    ASSERT_FALSE(fnDefault.tryCall<void>({}));
    ASSERT_FALSE(fnDefault.tryCall<void>({}, 123, true, false));
    ASSERT_EQ(fnDefault.tryCall<int>({}), std::nullopt);
    fnDefault.reset();
    ASSERT_FALSE(fnDefault.isAlive());
    ASSERT_EQ(fnDefault.owner(), PluginWeakRef{});
    ASSERT_FALSE(fnDefault.tryCall<void>({}));

    LuaMethodRef fnFoo(weak, (*state)["tbl"]["foo"], (*state)["tbl"]);
    ASSERT_TRUE(fnFoo.isAlive());
    ASSERT_EQ(fnFoo.owner(), weak);

    ASSERT_EQ(called, 0);
    ASSERT_TRUE(fnFoo.tryCall<void>({}));
    ASSERT_EQ(called, 1);
    ASSERT_TRUE(fnFoo.tryCall<void>({}, 123));
    ASSERT_EQ(called, 2);

    LuaMethodRef fnFooCopy = fnFoo;
    ASSERT_TRUE(fnFoo.isAlive());
    ASSERT_TRUE(fnFooCopy.isAlive());
    ASSERT_EQ(called, 2);
    ASSERT_TRUE(fnFooCopy.tryCall<void>({}));
    ASSERT_EQ(called, 3);

    LuaMethodRef fnFooMoved(std::move(fnFoo));
    ASSERT_FALSE(fnFoo.isAlive());
    ASSERT_TRUE(fnFooCopy.isAlive());
    ASSERT_TRUE(fnFooMoved.isAlive());
    ASSERT_EQ(fnFooMoved.owner(), weak);
    ASSERT_NE(fnFoo.owner(), weak);
    ASSERT_EQ(fnFoo.owner(), PluginWeakRef{});

    ASSERT_EQ(called, 3);
    ASSERT_FALSE(fnFoo.tryCall<void>({}));
    ASSERT_EQ(called, 3);
    ASSERT_TRUE(fnFooCopy.tryCall<void>({}));
    ASSERT_EQ(called, 4);
    ASSERT_TRUE(fnFooMoved.tryCall<void>({}));
    ASSERT_EQ(called, 5);

    LuaMethodRef fnFooMoveAssigned;
    ASSERT_FALSE(fnFooMoveAssigned.isAlive());
    fnFooMoveAssigned = std::move(fnFooMoved);

    ASSERT_FALSE(fnFooMoved.isAlive());
    ASSERT_TRUE(fnFooMoveAssigned.isAlive());

    ASSERT_EQ(called, 5);
    ASSERT_FALSE(fnFooMoved.tryCall<void>({}));
    ASSERT_EQ(called, 5);
    ASSERT_TRUE(fnFooMoveAssigned.tryCall<void>({}));
    ASSERT_EQ(called, 6);

    LuaMethodRef fnFooCopyAssigned;
    ASSERT_FALSE(fnFooCopyAssigned.isAlive());
    fnFooCopyAssigned = fnFooMoveAssigned;
    ASSERT_TRUE(fnFooCopyAssigned.isAlive());
    ASSERT_TRUE(fnFooMoveAssigned.isAlive());

    ASSERT_EQ(called, 6);
    ASSERT_TRUE(fnFooMoveAssigned.tryCall<void>({}));
    ASSERT_EQ(called, 7);
    ASSERT_TRUE(fnFooCopyAssigned.tryCall<void>({}));
    ASSERT_EQ(called, 8);

    fnFooCopyAssigned.reset();
    ASSERT_FALSE(fnFooCopyAssigned.isAlive());
    ASSERT_NE(fnFooCopyAssigned.owner(), weak);
    ASSERT_EQ(fnFooCopyAssigned.owner(), PluginWeakRef{});
    ASSERT_FALSE(fnFooCopyAssigned.tryCall<void>({}));
    ASSERT_EQ(called, 8);
}

}  // namespace chatterino::lua

#include "LuaUtilitiesTest.moc"
