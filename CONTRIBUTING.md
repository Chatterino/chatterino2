# Chatterino code guidelines

This is a set of guidelines for contributing to Chatterino. The goal is to teach programmers without a C++ background (java/python/etc.), people who haven't used Qt, or otherwise have different experience, the idioms of the codebase. Thus we will focus on those which are different from those other environments. There are extra guidelines available [here](https://hackmd.io/@fourtf/chatterino-pendantic-guidelines) but they are considered as extras and not as important.

### General (non-code related) guidelines for contributing to Chatterino

- Make a specific branch for your pull request instead of using the master, main, or mainline branch. This will prevent future problems with updating your branch after your PR is merged.

# Tooling

## Formatting

Code is automatically formatted using `clang-format`. It takes the burden off of the programmer and ensures that all contributors use the same style (even if mess something up accidentally). We recommend that you set up automatic formatting on file save in your editor.

# Comments

Comments should only be used to:

- Increase readability (e.g. grouping member variables).
- Containing information that can't be expressed in code.

Try to structure your code so that comments are not required.

#### Good example

```cpp
/// Result is 0 if a == b, negative if a < b and positive if b > a.
/// ^^^ You can't know this from the function signature!
//      Even better: Return a "strong ordering" type.
//          (but we don't have such a type right now)
int compare(const QString &a, const QString &b);
```

#### Bad example

```cpp
/*
 * Matches a link and returns std::nullopt if it failed and a
 * QRegularExpressionMatch on success.
 * ^^^ This comment just repeats the function signature!!!
 *
 * @param text The text that will be checked if it contains a
 * link
 * ^^^ No need to repeat the obvious.
 */
std::optional<QRegularExpressionMatch> matchLink(const QString &text);
```

# Code

## Arithmetic Types

Arithmetic types (like char, short, int, long, float and double), bool, and pointers are NOT initialized by default in c++. They keep whatever value is already at their position in the memory. This makes debugging harder and is unpredictable, so we initialize them to zero by using `{}` after their name when declaring them.

```cpp
class ArithmeticTypes
{
    int thisIs0{};
    QWidget *thisIsNull{};
    bool thisIsFalse_{};
    // int a; // <- Initialized to "random" value.
    // QWidget *randomPtr.

    std::vector<int> myVec; // <- other types call constructors instead, so no need for {}
    // std::vector<int> myVec{}; <- pointless {}

    int thisIs5 = 5; // <- Also fine, we initialize it with another value.
};

void myFunc() {
    int a = 1 + 1; // <- here we initialize it immediately, so it's fine.
}
```

## Passing parameters

The way a parameter is passed, signals how it is going to be used inside of the function. C++ doesn't have multiple return values, so there are "out parameters" (reference to a variable that is going to be assigned inside of the function) to simulate multiple return values.

**Cheap to copy types** like int/enum/etc. can be passed in per value since copying them is fast.

```cpp
void setValue(int value) {
    // ...
}
```

**References** mean that the variable doesn't need to be copied when it is passed to a function.

| type               | meaning                                                                                  |
| ------------------ | ---------------------------------------------------------------------------------------- |
| `const Type& name` | _in_ Parameter. It is NOT going to be modified and may be copied inside of the function. |
| `Type& name`       | _out_ or _in+out_ Parmameter. It will be modified.                                       |

**Pointers** signal that objects are managed manually. While the above are only guaranteed to live as long as the function call (= don't store and use later) these may have more complex lifetimes.

| type         | meaning                                                                                                                    |
| ------------ | -------------------------------------------------------------------------------------------------------------------------- |
| `Type* name` | The lifetime of the parameter may exceed the length of the function call. It may use the `QObject` parent/children system. |

**R-value references** `&&` work similar to regular references but signal the parameter should be "consumed".

```cpp
void storeLargeObject(LargeObject &&object) {
    // ...
}

void storeObject(std::unique_ptr<Object> &&object) {
    // ...
}

void main() {
    // initialize a large object (= will be expensive to copy)
    LargeObject large = // ...

    // Object accepts an r-value reference + we use std::move()
    // => We move the object = no need to copy.
    storeLargeObject(std::move(large));

    // But even worse, you can't copy a unique_ptr so we need to move here!
    std::unique_ptr<Object> unique = // ...
    storeObject(std::move(unique));

    // The pointer contained by unique has now been consumed by "storeObject"
    // so it just holds a null pointer now.
    assert(unique.get() == nullptr);
}
```

Generally the lowest level of requirement should be used, e.g. passing `Channel&` instead of `std::shared_ptr<Channel>&` (aka `ChannelPtr`) if possible.

## Members

All function names are in `camelCase`. _Private_ member variables are in `camelCase_` (note the underscore at the end). We don't use the `get` prefix for getters. We mark functions as `const` [if applicable](https://stackoverflow.com/questions/751681/meaning-of-const-last-in-a-function-declaration-of-a-class).

```cpp
class NamedObject
{
public:
    const QString &name() const; // <- no "get" prefix.
    void setName(const QString &name);
    bool hasLongName() const; // <- "has" or "is" prefix is okay

    static void myStaticFunction(); // <- also lowercase
    QString publicName;

private:
    // Private variables have "_" suffix.
    QString name_;
    // QString name; <- collides with name() function
};

void myFreeStandingFunction(); // <- also lower case
```

## Casts

- **Avoid** c-style casts: `(type)variable`.
- Instead use explicit type casts: `type(variable)`
- Or use one of [static_cast](https://en.cppreference.com/w/cpp/language/static_cast), [const_cast](https://en.cppreference.com/w/cpp/language/const_cast) and [dynamic_cast](https://en.cppreference.com/w/cpp/language/dynamic_cast)
- Try to avoid [reinterpret_cast](https://en.cppreference.com/w/cpp/language/reinterpret_cast) unless necessary.

```cpp
void example() {
    float f = 123.456;
    int i = (int)f; // <- don't
    int i = int(f); // <- do

    Base* base = // ...
    Derived* derived = (Derived*)base;               // <- don't
    Derived* derived = dynamic_cast<Derived*>(base); // <- do

    // Only use "const_cast" solved if using proper const correctness doesn't work.
    const int c = 123;
    ((int &)c) = 123;           // <- don't
    const_cast<int &>(c) = 123; // <- do (but only sometimes)

    // "reinterpret_cast" is also only required in very rarely.
    int p = 123;
    float *pp = (float*)&p;
    float *pp = reinterpret_cast<float*>(&p);
}
```

## This

Always use `this` to refer to instance members to make it clear where we use either locals or members.

```cpp
class Test
{
    void testFunc(int a);
    int testInt_{};
}

Test::testFunc(int a)
{
    // do
    this->testInt_ += 2;
    this->testFunc();

    // don't
    testInt_ -= 123;
    testFunc(2);
    this->testFunc(testInt_ + 1);
}
```

## Managing resources

#### Regular classes

Keep the element on the stack if possible. If you need a pointer or have complex ownership you should use one of these classes:

- Use `std::unique_ptr` if the resource has a single owner.
- Use `std::shared_ptr` if the resource has multiple owners.

#### QObject classes

- Use the [object tree](https://doc.qt.io/qt-5/objecttrees.html#) to manage lifetimes where possible. Objects are destroyed when their parent object is destroyed.
- If you have to explicitly delete an object use `variable->deleteLater()` instead of `delete variable`. This ensures that it will be deleted on the correct thread.
- If an object doesn't have a parent, consider using `std::unique_ptr<Type, DeleteLater>` with `DeleteLater` from "src/common/Common.hpp". This will call `deleteLater()` on the pointer once it goes out of scope, or the object is destroyed.

## Conventions

#### Usage strings

When informing the user about how a command is supposed to be used, we aim to follow [this standard](https://pubs.opengroup.org/onlinepubs/9699919799/basedefs/V1_chap12.html) where possible.

- Square brackets are reserved for `[optional arguments]`.
- Angle brackets are reserved for `<required arguments>`.
- The word _Usage_ should be capitalized and must be followed by a colon.
- If the usage deserves a description, put a dot after all parameters and explain it briefly.

##### Good

- `Usage: /block <user>`
- `Usage: /unblock <user>. Unblocks a user.`
- `Usage: /streamlink [channel]`
- `Usage: /usercard <user> [channel]`

##### Bad

- `Usage /streamlink <channel>` - Missing colon after _Usage_.
- `usage: /streamlink <channel>` - _Usage_ must be capitalized.
- `Usage: /streamlink channel` - The required argument `channel` must be wrapped in angle brackets.
- `Usage: /streamlink <channel>.` - Don't put a dot after usage if it's not followed by a description.
