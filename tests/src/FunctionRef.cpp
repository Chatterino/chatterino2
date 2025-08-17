#include "util/FunctionRef.hpp"

#include "Test.hpp"

#include <memory>

using namespace chatterino;

namespace {

size_t myFunc(const std::string &s)
{
    return s.size();
}

std::string myFunc2(int i)
{
    return std::to_string(i);
}

std::string refCaller(FunctionRef<std::string(int)> fn, int i)
{
    return fn(i);
}

}  // namespace

TEST(FunctionRef, lambda)
{
    static int i = 0;
    auto empty = []() {
        i++;
    };

    FunctionRef<void()>{empty}();
    ASSERT_EQ(i, 1);

    auto unique = std::make_unique<int>(0);
    int *up = unique.get();
    auto noncopyable = [p{std::move(unique)}]() {
        *p += 1;
    };
    FunctionRef<void()>{noncopyable}();
    ASSERT_EQ(*up, 1);
    {
        FunctionRef<void()> f{std::move(noncopyable)};
        f();
        ASSERT_EQ(*up, 2);
    }

    auto identity = [](auto &&it) {
        return std::forward<decltype(it)>(it);
    };

    ASSERT_EQ(FunctionRef<int(int)>{identity}(1), 1);
    ASSERT_EQ(
        FunctionRef<std::string(std::string)>{identity}(std::string{"foo"}),
        "foo");

    ASSERT_EQ(refCaller(
                  [](int i) {
                      return std::to_string(i);
                  },
                  42),
              "42");

    ASSERT_EQ(refCaller(
                  [p{std::make_unique<int>(1)}](int i) {
                      return std::to_string(i + *p);
                  },
                  42),
              "43");
}

TEST(FunctionRef, pointer)
{
    ASSERT_EQ(FunctionRef<int(std::string)>{myFunc}("foo"), 3);
    ASSERT_EQ(FunctionRef<std::string(int)>{myFunc2}(2), "2");
    ASSERT_EQ(refCaller(myFunc2, 42), "42");
}

TEST(FunctionRef, stdFunction)
{
    int i = 0;
    FunctionRef<void()>{std::function{[&] {
        i++;
    }}}();
    ASSERT_EQ(i, 1);
    ASSERT_EQ(FunctionRef<size_t(std::string)>{std::function{myFunc}}("foo"),
              3);
}

TEST(FunctionRef, operatorEq)
{
    int i = 0;
    FunctionRef<void()> f1([] {});
    FunctionRef<void()> f2([&] {
        i++;
    });
    FunctionRef<void()> f3([=]() mutable {
        i++;
    });
    ASSERT_EQ(f1, f1);
    ASSERT_NE(f1, f2);
    ASSERT_NE(f1, f3);
    ASSERT_EQ(f2, f2);
    ASSERT_NE(f2, f1);
    ASSERT_NE(f2, f3);
    ASSERT_EQ(f3, f3);
    ASSERT_NE(f3, f1);
    ASSERT_NE(f3, f2);

    FunctionRef<size_t(std::string)> f4(myFunc);
    FunctionRef<size_t(std::string)> f5(myFunc);
    FunctionRef<size_t(std::string)> f6([](std::string /*s*/) {  // NOLINT
        return 0;
    });

    ASSERT_EQ(f4, f4);
    ASSERT_EQ(f4, f5);
    ASSERT_EQ(f5, f4);
    ASSERT_NE(f4, f6);
    ASSERT_NE(f6, f4);
    ASSERT_EQ(f6, f6);
}

TEST(FunctionRef, operatorBool)
{
    FunctionRef<void()> f0;
    FunctionRef<void()> f1([] {});
    FunctionRef<size_t(std::string)> f2(myFunc);

    ASSERT_FALSE(f0);
    ASSERT_TRUE(f1);
    ASSERT_TRUE(f2);
}

TEST(FunctionRef, copyCtor)
{
    FunctionRef<void()> f0;
    auto f0Copy = f0;
    ASSERT_EQ(f0, f0Copy);
    ASSERT_FALSE(f0);
    ASSERT_FALSE(f0Copy);

    int i = 0;
    auto cb = [&] {
        i++;
    };
    FunctionRef<void()> f1(cb);
    auto f1Copy = f1;
    ASSERT_EQ(f1, f1Copy);
    f1();
    ASSERT_EQ(i, 1);
    f1Copy();
    ASSERT_EQ(i, 2);

    FunctionRef<size_t(std::string)> f2(myFunc);
    auto f2Copy = f2;
    ASSERT_EQ(f2, f2Copy);
    ASSERT_EQ(f2("foobar"), f2Copy("foobar"));

    static_assert(std::is_trivially_copyable_v<decltype(f0)>);
    static_assert(std::is_trivially_copyable_v<decltype(f2)>);

    static_assert(std::is_trivially_move_assignable_v<decltype(f0)>);
    static_assert(std::is_trivially_move_assignable_v<decltype(f2)>);

    static_assert(std::is_trivially_move_constructible_v<decltype(f0)>);
    static_assert(std::is_trivially_move_constructible_v<decltype(f2)>);
}
