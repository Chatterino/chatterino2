#include "common/FlagsEnum.hpp"

#include "Test.hpp"

using namespace chatterino;

namespace {

enum class BasicScoped : std::uint16_t {
    None = 0,
    Foo = 1 << 0,
    Bar = 1 << 1,
    Baz = 1 << 2,
    Qox = 1 << 3,
    Quux = 1 << 4,
    Corge = 1 << 5,
    Grault = 1 << 6,
    Garply = 1 << 7,
    Waldo = 1 << 8,
    Fred = 1 << 9,
};
using BasicInt = std::underlying_type_t<BasicScoped>;

enum BasicUnscoped : BasicInt {
    None = 0,
    Foo = 1 << 0,
    Bar = 1 << 1,
    Baz = 1 << 2,
    Qox = 1 << 3,
    Quux = 1 << 4,
    Corge = 1 << 5,
    Grault = 1 << 6,
    Garply = 1 << 7,
    Waldo = 1 << 8,
    Fred = 1 << 9,
};

using Scoped = FlagsEnum<BasicScoped>;
using Unscoped = FlagsEnum<BasicUnscoped>;

}  // namespace

TEST(FlagsEnum, sizeAndAlign)
{
    enum class U8 : std::uint8_t {};
    enum class I8 : std::int8_t {};
    enum class U16 : std::uint16_t {};
    enum class I16 : std::int16_t {};
    enum class U32 : std::uint32_t {};
    enum class I32 : std::int32_t {};
    enum class U64 : std::uint64_t {};
    enum class I64 : std::int64_t {};

    auto check = []<typename... T>() {
        return (((sizeof(T) == sizeof(FlagsEnum<T>)) && ...) &&
                ((alignof(T) == alignof(FlagsEnum<T>)) && ...));
    };

    static_assert(
        check.template operator()<U8, I8, U16, I16, U32, I32, U64, I64>());
}

template <typename E>
consteval void testCtor()
{
    using FE = FlagsEnum<E>;
    using U = std::underlying_type_t<E>;

    static_assert(FE{}.value() == E::None);
    static_assert(FE{E::Bar}.value() == E::Bar);
    static_assert(
        FE{E::Bar, E::Qox}.value() ==
        static_cast<E>(static_cast<U>(E::Bar) | static_cast<U>(E::Qox)));
    static_assert(
        FE{E::Bar, E::Bar, E::Qox}.value() ==
        static_cast<E>(static_cast<U>(E::Bar) | static_cast<U>(E::Qox)));
}

TEST(FlagsEnum, ctor)
{
    testCtor<BasicScoped>();
    testCtor<BasicUnscoped>();
}

template <typename E>
consteval void testOperatorEq()
{
    using FE = FlagsEnum<E>;

    static_assert(FE{} == FE{});

    static_assert(FE{E::Corge} == FE{E::Corge});

    static_assert(FE{E::Corge, E::Garply} == FE{E::Garply, E::Corge});

    static_assert(FE{} == E::None);
    static_assert(E::None == FE{});
    static_assert(FE{E::Foo} == E::Foo);
    static_assert(E::Foo == FE{E::Foo});
}

TEST(FlagsEnum, operatorEq)
{
    testOperatorEq<BasicScoped>();
    testOperatorEq<BasicUnscoped>();
}

template <typename E>
consteval void testOperatorNeq()
{
    using FE = FlagsEnum<E>;

    static_assert(FE{} != FE{E::Quux});

    static_assert(FE{E::Corge} != FE{E::Grault});

    static_assert(FE{E::Corge, E::Garply} != FE{E::Garply, E::Waldo});

    static_assert(FE{} != E::Foo);
    static_assert(E::Foo != FE{});
    static_assert(FE{E::Foo} != E::None);
    static_assert(E::None != FE{E::Foo});
}

TEST(FlagsEnum, operatorNeq)
{
    testOperatorNeq<BasicScoped>();
    testOperatorNeq<BasicUnscoped>();
}

template <typename E>
inline void testSetUnset()
{
    using FE = FlagsEnum<E>;

    FE s;
    ASSERT_EQ(s, FE{});
    s.set(E::Foo);
    ASSERT_EQ(s, E::Foo);
    s.set(E::Fred, E::Qox);
    ASSERT_EQ(s, (FE{E::Foo, E::Fred, E::Qox}));
    s.set(E::Foo);
    ASSERT_EQ(s, (FE{E::Foo, E::Fred, E::Qox}));

    s.set(FE{E::Bar, E::Baz, E::Qox});
    ASSERT_EQ(s, (FE{E::Foo, E::Fred, E::Qox, E::Bar, E::Baz}));

    s.set(E::Foo, false);
    ASSERT_EQ(s, (FE{E::Fred, E::Qox, E::Bar, E::Baz}));

    s.unset(E::Foo, E::Qox, E::Bar);
    ASSERT_EQ(s, (FE{E::Fred, E::Baz}));

    s.unset(E::Fred, E::Baz, E ::Bar);
    ASSERT_EQ(s, FE{});

    s.unset(E::Baz);
    ASSERT_EQ(s, FE{});

    static_assert([] {
        FE s;
        bool result = s == FE{};

        s.set(E::Foo);
        result = result && s == E::Foo;

        s.set(E::Fred, E::Qox);
        result = result && s == FE{E::Foo, E::Fred, E::Qox};

        s.set(E::Foo);
        result = result && s == FE{E::Foo, E::Fred, E::Qox};

        s.set(FE{E::Bar, E::Baz, E::Qox});
        result = result && s == FE{E::Foo, E::Fred, E::Qox, E::Bar, E::Baz};

        s.set(E::Foo, false);
        result = result && s == (FE{E::Fred, E::Qox, E::Bar, E::Baz});

        s.unset(E::Foo, E::Qox, E::Bar);
        result = result && s == (FE{E::Fred, E::Baz});

        s.unset(E::Fred, E::Baz, E ::Bar);
        result = result && s == (FE{});

        s.unset(E::Baz);
        result = result && s == (FE{});

        return result;
    }());
}

TEST(FlagsEnum, setUnset)
{
    testSetUnset<BasicScoped>();
    testSetUnset<BasicUnscoped>();
}

template <typename E>
consteval void testOperatorBitOr()
{
    using FE = FlagsEnum<E>;

    static_assert((FE{E::Foo} | E::Bar) == FE{E::Foo, E::Bar});
    static_assert((FE{E::Foo} | E::Foo) == FE{E::Foo});
    static_assert((FE{E::Foo} | E::None) == FE{E::Foo});
    static_assert((FE{} | E::Foo) == FE{E::Foo});
    static_assert((FE{} | E::None) == FE{});

    static_assert((FE{E::Foo} | FE{E::Bar}) == FE{E::Foo, E::Bar});
    static_assert((FE{E::Foo} | FE{E::Foo, E::Bar, E::Baz}) ==
                  FE{E::Foo, E::Bar, E::Baz});
    static_assert((FE{E::Foo} | FE{}) == FE{E::Foo});
    static_assert((FE{} | FE{}) == FE{});
}

TEST(FlagsEnum, operatorBitOr)
{
    testOperatorBitOr<BasicScoped>();
    testOperatorBitOr<BasicUnscoped>();
}

template <typename E>
consteval void testHas()
{
    using FE = FlagsEnum<E>;

    static_assert(FE{E::Foo}.has(E::Foo));
    static_assert(!FE{E::Foo}.has(E::Bar));
    static_assert(!FE{E::Foo}.has(E::None));

    static_assert(!FE{}.has(E::Foo));
    static_assert(!FE{}.has(E::None));

    static_assert(FE{E::Qox, E::Waldo, E::Garply}.has(E::Qox));
    static_assert(FE{E::Qox, E::Waldo, E::Garply}.has(E::Waldo));
    static_assert(FE{E::Qox, E::Waldo, E::Garply}.has(E::Garply));
    static_assert(!FE{E::Qox, E::Waldo, E::Garply}.has(E::Grault));
}

TEST(FlagsEnum, has)
{
    testHas<BasicScoped>();
    testHas<BasicUnscoped>();
}

template <typename E>
consteval void testHasAny()
{
    using FE = FlagsEnum<E>;

    static_assert(FE{E::Foo}.hasAny(E::Foo));
    static_assert(!FE{E::Foo}.hasAny(E::Bar));
    static_assert(!FE{E::Foo}.hasAny(E::None));

    static_assert(!FE{}.hasAny(E::Foo));
    static_assert(!FE{}.hasAny(E::None));

    static_assert(FE{E::Qox, E::Waldo, E::Garply}.hasAny(E::Qox, E::Foo));
    static_assert(FE{E::Qox, E::Waldo, E::Garply}.hasAny({E::Qox, E::Foo}));
    static_assert(FE{E::Qox, E::Waldo, E::Garply}.hasAny(E::Waldo, E::None));
    static_assert(FE{E::Qox, E::Waldo, E::Garply}.hasAny(E::Garply, E::Grault));
    static_assert(!FE{E::Qox, E::Waldo, E::Garply}.hasAny(E::Grault));
    static_assert(!FE{E::Qox, E::Waldo, E::Garply}.hasAny());
}

TEST(FlagsEnum, hasAny)
{
    testHasAny<BasicScoped>();
    testHasAny<BasicUnscoped>();
}

template <typename E>
consteval void testHasAll()
{
    using FE = FlagsEnum<E>;

    static_assert(FE{E::Foo}.hasAll(E::Foo));
    static_assert(FE{E::Foo}.hasAll(E::None));
    static_assert(!FE{E::Foo}.hasAll(E::Bar));

    static_assert(FE{}.hasAll(E::None));
    static_assert(!FE{}.hasAll(E::Foo));

    static_assert(FE{E::Qox, E::Waldo, E::Garply}.hasAll(E::Waldo, E::None));
    static_assert(FE{E::Qox, E::Waldo, E::Garply}.hasAll({E::Waldo, E::None}));
    static_assert(FE{E::Qox, E::Waldo, E::Garply}.hasAll());
    static_assert(
        FE{E::Qox, E::Waldo, E::Garply}.hasAll(E::Qox, E::Waldo, E::Garply));
    static_assert(FE{E::Qox, E::Waldo, E::Garply}.hasAll(E::Qox, E::Waldo,
                                                         E::Garply, E::None));

    static_assert(!FE{E::Qox, E::Waldo, E::Garply}.hasAll(E::Qox, E::Foo));
    static_assert(
        !FE{E::Qox, E::Waldo, E::Garply}.hasAll(E::Garply, E::Grault));
    static_assert(!FE{E::Qox, E::Waldo, E::Garply}.hasAll(E::Grault));
    static_assert(!FE{E::Qox, E::Waldo, E::Garply}.hasAll(E::Qox, E::Waldo,
                                                          E::Garply, E::Foo));
}

TEST(FlagsEnum, hasAll)
{
    testHasAll<BasicScoped>();
    testHasAll<BasicUnscoped>();
}

template <typename E>
consteval void testHasNone()
{
    using FE = FlagsEnum<E>;

    static_assert(FE{E::Foo}.hasNone(E::Bar));
    static_assert(FE{E::Foo}.hasNone(E::None));

    static_assert(FE{}.hasNone(E::Foo));
    static_assert(FE{}.hasNone(E::None));

    static_assert(FE{E::Qox, E::Waldo, E::Garply}.hasNone(E::Foo, E::Foo));
    static_assert(FE{E::Qox, E::Waldo, E::Garply}.hasNone({E::Bar, E::Foo}));
    static_assert(!FE{E::Qox, E::Waldo, E::Garply}.hasNone(E::Waldo, E::Foo));
    static_assert(!FE{E::Qox, E::Waldo, E::Garply}.hasNone(E::Garply));
}

TEST(FlagsEnum, hasNone)
{
    testHasNone<BasicScoped>();
    testHasNone<BasicUnscoped>();
}

template <typename E>
consteval void testIsEmpty()
{
    using FE = FlagsEnum<E>;

    static_assert(FE{}.isEmpty());
    static_assert(!FE{E::Foo}.isEmpty());
    static_assert(FE{E::None}.isEmpty());
    static_assert(!FE{E::Foo, E::Waldo}.isEmpty());
}

TEST(FlagsEnum, isEmpty)
{
    testIsEmpty<BasicScoped>();
    testIsEmpty<BasicUnscoped>();
}

template <typename T>
constexpr inline auto CONSTRUCTION_VALID = requires() { FlagsEnum<T>{}; };

TEST(FlagsEnum, templateParam)
{
    struct S {
    };

    static_assert(!CONSTRUCTION_VALID<S>);
    static_assert(!CONSTRUCTION_VALID<int>);
    static_assert(!CONSTRUCTION_VALID<bool>);
    static_assert(!CONSTRUCTION_VALID<BasicScoped &>);
    static_assert(!CONSTRUCTION_VALID<BasicUnscoped &>);

    static_assert(CONSTRUCTION_VALID<BasicScoped>);
    static_assert(CONSTRUCTION_VALID<BasicUnscoped>);
}
