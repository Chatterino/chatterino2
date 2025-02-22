#pragma once

#include <boost/json.hpp>
#include <QAnyStringView>
#include <QString>

#include <string_view>

// NOLINTBEGIN(cppcoreguidelines-pro-type-union-access)
// NOLINTBEGIN(bugprone-undefined-memory-manipulation)
// NOLINTBEGIN(cppcoreguidelines-avoid-c-arrays)
namespace chatterino::eventsub::lib {

/// String is a struct that holds either an UTF-8 string or a QString
///
/// The intended use is for it to receive an UTF-8 string that has been built by
/// boost::json, and once it's been passed into a GUI appliciaton, it can use
/// the `qt` function to convert the backing string to a QString,
/// while we ensure the conversion is only done once.
/// The string can always be viewed using \ref view() in Qt6.
///
/// It uses the small-string-optimization and has `sizeof(QString)` bytes of
/// storage it can use without allocating (Qt6: 24B, Qt5: 8B). The stored string
/// is **not** null-terminated.
struct String {
    constexpr String() noexcept = default;
    String(std::string_view sv)
        : flags(sv.length() & LENGTH_MASK)
    {
        char *data = this->storage.inPlace;

        if (sv.length() > SSO_CAPACITY)
        {
            data = new char[sv.length()];
            this->flags |= ALLOC_BIT;
            this->storage.data = data;
        }

        for (size_t i = 0; i < sv.length(); i++)
        {
            data[i] = sv[i];
        }
    }

    ~String()
    {
        if (this->isQt())
        {
            this->storage.qt.~QString();
        }
        else if (this->isAlloc())
        {
            delete[] this->storage.data;
        }
        // else: inPlace
    }

    String(const String &s) = delete;
    String &operator=(const String &) = delete;

    String(String &&other) noexcept
        : storage(std::move(other.storage))
        , flags(other.flags)
    {
        other.flags = 0;
    }

    String &operator=(String &&other) noexcept
    {
        // clear our data (if needed)
        if (this->isQt())
        {
            this->storage.qt.~QString();
        }
        else if (this->isAlloc())
        {
            delete[] this->storage.data;
        }

        this->storage = std::move(other.storage);
        this->flags = other.flags;
        other.flags = 0;

        return *this;
    }

    /// Returns the string as a QString, modifying the backing string to ensure
    /// the copy only happens once.
    QString qt() const
    {
        if (this->isEmpty())
        {
            return {};
        }

        if (this->isQt())
        {
            return this->storage.qt;
        }

        // not a QString yet
        if (this->isAlloc())
        {
            auto s = QString::fromUtf8(
                this->storage.data,
                static_cast<qsizetype>(this->flags & LENGTH_MASK));
            delete[] this->storage.data;
            new (&this->storage.qt) QString(std::move(s));
            this->flags &= ~ALLOC_BIT;
        }
        else
        {
            auto s = QString::fromUtf8(
                this->storage.inPlace,
                static_cast<qsizetype>(this->flags & LENGTH_MASK));
            new (&this->storage.qt) QString(std::move(s));
        }
        this->flags |= QT_BIT;

        return this->storage.qt;
    }

    constexpr QAnyStringView view() const noexcept
    {
        if (this->isQt())
        {
            return this->storage.qt;
        }
        return {
            this->isAlloc() ? this->storage.data : this->storage.inPlace,
            static_cast<qsizetype>(this->flags & LENGTH_MASK),
        };
    }

    constexpr bool isEmpty() const noexcept
    {
        return this->flags == 0;
    }

    constexpr bool isQt() const noexcept
    {
        return (this->flags & QT_BIT) != 0;
    }

    constexpr bool isAlloc() const noexcept
    {
        return (this->flags & ALLOC_BIT) != 0;
    }

    constexpr bool isInPlace() const noexcept
    {
        return (this->flags & (ALLOC_BIT | QT_BIT)) == 0;
    }

    // note: because we're using C++ 20, the reversed operator
    // (e.g. QAnyStringView == String) is automatically "synthesized".
    bool operator==(const QAnyStringView &other) const noexcept
    {
        return this->view() == other;
    }

    bool operator==(const String &other) const noexcept
    {
        return this->view() == other.view();
    }

    template <typename = void>  // weak overload
    bool operator==(const std::string_view &other) const noexcept
    {
        return this->view() == other;
    }

private:
    static constexpr size_t QT_BIT = 1ULL << (sizeof(size_t) * 8 - 1);
    static constexpr size_t ALLOC_BIT = 1ULL << (sizeof(size_t) * 8 - 2);
    static constexpr size_t LENGTH_MASK = std::min(ALLOC_BIT, QT_BIT) - 1;
    static constexpr size_t SSO_CAPACITY = sizeof(QString);

    static_assert((LENGTH_MASK & ALLOC_BIT) == 0);
    static_assert((LENGTH_MASK & QT_BIT) == 0);

    mutable union Storage {
        constexpr Storage() noexcept
        {
        }
        ~Storage() noexcept
        {
        }

        Storage(const Storage &) = delete;
        Storage &operator=(const Storage &) = delete;

        Storage(Storage &&other) noexcept
        {
            Storage::move(std::addressof(other), this);
        };
        Storage &operator=(Storage &&other) noexcept
        {
            Storage::move(std::addressof(other), this);
            return *this;
        };

        char inPlace[SSO_CAPACITY]{};
        const char *data;
        QString qt;

    private:
        static void move(Storage *from, Storage *to)
        {
            // we can memcpy QStrings as they're relocatable
            static_assert(QTypeInfo<QString>::isRelocatable != 0);

            // copy `from` -> `to`
            std::memcpy(static_cast<void *>(to), from, sizeof(Storage));
#ifndef NDEBUG
            // Mark `from` as unused.
            // Because the parent `String` sets the flags to 0,
            // we don't _need_ to overwrite the data. In debug mode we write a
            // tombstone value here.
            std::memset(static_cast<void *>(from), 0xff, sizeof(Storage));
#endif
        }
    } storage;
    static_assert(sizeof(Storage) == sizeof(QString));

    /// Flags both store the length as well as the current state of the string.
    /// They're defined as follows (big endian, MSB is first, 64bit):
    /// ```
    /// ┌──────┬──────┬─────────┬─────────────────────────────────────────┐
    /// │Bit(s)│ 63   │ 62      │ 61..0                                   │
    /// ├──────┼──────┼─────────┼─────────────────────────────────────────┤
    /// │Value │ isQt │ isAlloc │ length                                  │
    /// └──────┴──────┴─────────┴─────────────────────────────────────────┘
    /// ```
    mutable size_t flags = 0;
};

boost::json::result_for<String, boost::json::value>::type tag_invoke(
    boost::json::try_value_to_tag<String>, const boost::json::value &jvRoot);

}  // namespace chatterino::eventsub::lib
// NOLINTEND(cppcoreguidelines-avoid-c-arrays)
// NOLINTEND(bugprone-undefined-memory-manipulation)
// NOLINTEND(cppcoreguidelines-pro-type-union-access)
