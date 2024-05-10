#pragma once

#include <QStringView>
#include <QtGlobal>
#include <QUtf8StringView>

namespace chatterino {

/// Case insensitive transparent comparator for Qt's string types
struct QCompareCaseInsensitive {
    using is_transparent = void;

    // clang-format off
    bool operator()(const QString &   a, const QString &   b) const noexcept;
    bool operator()(QStringView       a, QStringView       b) const noexcept;
    bool operator()(QLatin1StringView a, QLatin1StringView b) const noexcept;

    bool operator()(const QString &   a, QStringView       b) const noexcept;
    bool operator()(const QString &   a, QLatin1StringView b) const noexcept;

    bool operator()(QStringView       a, const QString &   b) const noexcept;
    bool operator()(QLatin1StringView a, const QString &   b) const noexcept;

    bool operator()(QStringView       a, QLatin1StringView b) const noexcept;
    bool operator()(QLatin1StringView a, QStringView       b) const noexcept;

#if QT_VERSION >= QT_VERSION_CHECK(6, 5, 0)
    bool operator()(QUtf8StringView   a, QUtf8StringView   b) const noexcept;

    bool operator()(const QString &   a, QUtf8StringView   b) const noexcept;
    bool operator()(QStringView       a, QUtf8StringView   b) const noexcept;
    bool operator()(QLatin1StringView a, QUtf8StringView   b) const noexcept;

    bool operator()(QUtf8StringView   a, const QString &   b) const noexcept;
    bool operator()(QUtf8StringView   a, QStringView       b) const noexcept;
    bool operator()(QUtf8StringView   a, QLatin1StringView b) const noexcept;
#endif
    // clang-format on
};

inline bool QCompareCaseInsensitive::operator()(const QString &a,
                                                const QString &b) const noexcept
{
    return a.compare(b, Qt::CaseInsensitive) < 0;
}

inline bool QCompareCaseInsensitive::operator()(QStringView a,
                                                QStringView b) const noexcept
{
    return a.compare(b, Qt::CaseInsensitive) < 0;
}

inline bool QCompareCaseInsensitive::operator()(
    QLatin1StringView a, QLatin1StringView b) const noexcept
{
    return a.compare(b, Qt::CaseInsensitive) < 0;
}

inline bool QCompareCaseInsensitive::operator()(const QString &a,
                                                QStringView b) const noexcept
{
    return a.compare(b, Qt::CaseInsensitive) < 0;
}

inline bool QCompareCaseInsensitive::operator()(
    const QString &a, QLatin1StringView b) const noexcept
{
    return a.compare(b, Qt::CaseInsensitive) < 0;
}

inline bool QCompareCaseInsensitive::operator()(QStringView a,
                                                const QString &b) const noexcept
{
    return a.compare(b, Qt::CaseInsensitive) < 0;
}

inline bool QCompareCaseInsensitive::operator()(QLatin1StringView a,
                                                const QString &b) const noexcept
{
    return a.compare(b, Qt::CaseInsensitive) < 0;
}

inline bool QCompareCaseInsensitive::operator()(
    QStringView a, QLatin1StringView b) const noexcept
{
    return a.compare(b, Qt::CaseInsensitive) < 0;
}

inline bool QCompareCaseInsensitive::operator()(QLatin1StringView a,
                                                QStringView b) const noexcept
{
    return a.compare(b, Qt::CaseInsensitive) < 0;
}

#if QT_VERSION >= QT_VERSION_CHECK(6, 5, 0)
inline bool QCompareCaseInsensitive::operator()(
    QUtf8StringView a, QUtf8StringView b) const noexcept
{
    return a.compare(b, Qt::CaseInsensitive) < 0;
}

inline bool QCompareCaseInsensitive::operator()(
    const QString &a, QUtf8StringView b) const noexcept
{
    return QStringView{a}.compare(b, Qt::CaseInsensitive) < 0;
}

inline bool QCompareCaseInsensitive::operator()(
    QStringView a, QUtf8StringView b) const noexcept
{
    return a.compare(b, Qt::CaseInsensitive) < 0;
}

inline bool QCompareCaseInsensitive::operator()(
    QLatin1StringView a, QUtf8StringView b) const noexcept
{
    return a.compare(b, Qt::CaseInsensitive) < 0;
}

inline bool QCompareCaseInsensitive::operator()(QUtf8StringView a,
                                                const QString &b) const noexcept
{
    return a.compare(b, Qt::CaseInsensitive) < 0;
}

inline bool QCompareCaseInsensitive::operator()(QUtf8StringView a,
                                                QStringView b) const noexcept
{
    return a.compare(b, Qt::CaseInsensitive) < 0;
}

inline bool QCompareCaseInsensitive::operator()(
    QUtf8StringView a, QLatin1StringView b) const noexcept
{
    return a.compare(b, Qt::CaseInsensitive) < 0;
}
#endif

}  // namespace chatterino
