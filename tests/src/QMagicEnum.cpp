#include "util/QMagicEnum.hpp"

#include "common/FlagsEnum.hpp"
#include "common/Literals.hpp"
#include "Test.hpp"

using namespace chatterino;
using namespace literals;

using qmagicenum::enumCast;
using qmagicenum::enumFlagsName;
using qmagicenum::enumName;
using qmagicenum::enumNames;
using qmagicenum::enumNameString;

namespace {

enum class MyEnum {
    Foo,
    Bar,
    Baz,
};

enum class MyFlag {
    None = 0,
    One = 1,
    Two = 2,
    Four = 4,
    Eight = 8,
    TwoPow9 = 512,
    TwoPow10 = 1024,
};
using MyFlags = chatterino::FlagsEnum<MyFlag>;

enum class MyCustom {
    Default = 1,
    First = 4,
    Second = 9,
};

enum MyOpen {
    OpenOne = 11,
    OpenTwo = 12,
    OpenThree = 13,
};

consteval bool eq(QStringView a, QStringView b)
{
    return qmagicenum::detail::eq(a, b, std::equal_to<>());
}

template <typename E>
consteval bool checkConst(E value, QStringView expectedName)
{
    return eq(enumName(value), expectedName) &&
           enumCast<E>(expectedName) == value;
}

template <typename E>
consteval bool checkInsensitive(E value, QStringView possible)
{
    return enumCast<E>(possible, qmagicenum::CASE_INSENSITIVE) == value;
}

template <typename E, std::size_t N = enumNames<E>().size()>
consteval bool checkValues(std::array<QStringView, N> values)
{
    constexpr auto got = enumNames<E>();
    if (got.size() != N)
    {
        return false;
    }
    for (size_t i = 0; i < N; i++)
    {
        if (!eq(got.at(i), values.at(i)))
        {
            return false;
        }
    }
    return true;
}

}  // namespace

template <>
struct magic_enum::customize::enum_range<MyFlag> {
    static constexpr bool is_flags = true;  // NOLINT
};

template <>
constexpr magic_enum::customize::customize_t
    magic_enum::customize::enum_name<MyCustom>(MyCustom value) noexcept
{
    switch (value)
    {
        case MyCustom::First:
            return "myfirst";
        case MyCustom::Second:
            return "mysecond.*";

        default:
            return default_tag;
    }
}

TEST(QMagicEnum, basic)
{
    static_assert(eq(enumName<MyEnum::Foo>(), u"Foo"));
    static_assert(eq(enumName<MyEnum::Bar>(), u"Bar"));
    static_assert(eq(enumName<MyEnum::Baz>(), u"Baz"));
    static_assert(checkConst(MyEnum::Foo, u"Foo"));
    static_assert(checkConst(MyEnum::Bar, u"Bar"));
    static_assert(checkConst(MyEnum::Baz, u"Baz"));
    static_assert(eq(enumName(static_cast<MyEnum>(16)), u""));
    static_assert(checkValues<MyEnum>({u"Foo", u"Bar", u"Baz"}));
}

TEST(QMagicEnum, flags)
{
    static_assert(eq(enumName<MyFlag::None>(), u"None"));
    static_assert(eq(enumName<MyFlag::One>(), u"One"));
    static_assert(eq(enumName<MyFlag::Two>(), u"Two"));
    static_assert(eq(enumName<MyFlag::Four>(), u"Four"));
    static_assert(eq(enumName<MyFlag::Eight>(), u"Eight"));

    static_assert(!magic_enum::enum_index<MyFlag>(MyFlag::None).has_value());
    static_assert(eq(enumName(MyFlag::None), u""));

    static_assert(checkConst(MyFlag::One, u"One"));
    static_assert(checkConst(MyFlag::Two, u"Two"));
    static_assert(checkConst(MyFlag::Four, u"Four"));
    static_assert(checkConst(MyFlag::Eight, u"Eight"));
    static_assert(checkConst(MyFlag::Eight, u"Eight"));
    static_assert(eq(enumName(static_cast<MyFlag>(16)), u""));
    static_assert(checkValues<MyFlag>(
        {u"One", u"Two", u"Four", u"Eight", u"TwoPow9", u"TwoPow10"}));
}

TEST(QMagicEnum, enumNameString)
{
    ASSERT_EQ(enumNameString<MyEnum::Baz>(), u"Baz");

    ASSERT_EQ(enumNameString<MyFlag::None>(), u"None");
    ASSERT_EQ(enumNameString<MyFlag::Four>(), u"Four");

    ASSERT_EQ(enumNameString(MyEnum::Bar), u"Bar");
    ASSERT_EQ(enumNameString(MyFlag::None), u"");
    ASSERT_EQ(enumNameString(MyFlag::One), u"One");
    ASSERT_EQ(enumNameString(MyCustom::Second), u"mysecond.*");
    ASSERT_EQ(enumNameString(OpenTwo), u"OpenTwo");
}

TEST(QMagicEnum, enumFlagsName)
{
    ASSERT_EQ(enumFlagsName(MyFlag::Eight), u"Eight"_s);
    ASSERT_EQ(enumFlagsName(MyFlag::None), u""_s);
    ASSERT_EQ(enumFlagsName(MyFlags{MyFlag::Eight, MyFlag::Four}.value(), u'+'),
              u"Four+Eight"_s);
    ASSERT_EQ(enumFlagsName(
                  MyFlags{MyFlag::Eight, MyFlag::One, MyFlag::Two, MyFlag::Four}
                      .value()),
              u"One|Two|Four|Eight"_s);
    ASSERT_EQ(
        enumFlagsName(MyFlags{MyFlag::One, static_cast<MyFlag>(16)}.value()),
        u""_s);
}

TEST(QMagicEnum, renamed)
{
    static_assert(eq(enumName<MyCustom::Default>(), u"Default"));
    static_assert(eq(enumName<MyCustom::First>(), u"myfirst"));
    static_assert(eq(enumName<MyCustom::Second>(), u"mysecond.*"));
    static_assert(checkConst(MyCustom::Default, u"Default"));
    static_assert(checkConst(MyCustom::First, u"myfirst"));
    static_assert(checkConst(MyCustom::Second, u"mysecond.*"));
    static_assert(eq(enumName(static_cast<MyCustom>(16)), u""));
    static_assert(
        checkValues<MyCustom>({u"Default", u"myfirst", u"mysecond.*"}));
}

TEST(QMagicEnum, open)
{
    static_assert(eq(enumName<OpenOne>(), u"OpenOne"));
    static_assert(eq(enumName<OpenTwo>(), u"OpenTwo"));
    static_assert(eq(enumName<OpenThree>(), u"OpenThree"));
    static_assert(checkConst(OpenOne, u"OpenOne"));
    static_assert(checkConst(OpenTwo, u"OpenTwo"));
    static_assert(checkConst(OpenThree, u"OpenThree"));
    static_assert(eq(enumName(static_cast<MyOpen>(16)), u""));
    static_assert(checkValues<MyOpen>({u"OpenOne", u"OpenTwo", u"OpenThree"}));
}

TEST(QMagicEnum, caseInsensitive)
{
    static_assert(checkInsensitive(MyEnum::Foo, u"foo"));
    static_assert(checkInsensitive(MyEnum::Bar, u"BAR"));
    static_assert(checkInsensitive(MyFlag::Four, u"fOUR"));
    static_assert(checkInsensitive(MyCustom::Second, u"MySecond.*"));
    static_assert(checkInsensitive(OpenOne, u"openone"));
}
