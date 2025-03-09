#pragma once

#include <QLatin1String>
#include <QString>
#include <QStringView>
#include <QtGlobal>

#if QT_VERSION >= QT_VERSION_CHECK(6, 5, 0)
#    include <QUtf8StringView>
#endif

namespace chatterino {

template <Qt::CaseSensitivity CS>
struct QCompareTransparentBase {
    using is_transparent = void;

    // clang-format off
    bool operator()(const QString & a, const QString & b) const noexcept;
    bool operator()(QStringView     a, QStringView     b) const noexcept;
    bool operator()(QLatin1String   a, QLatin1String   b) const noexcept;

    bool operator()(const QString & a, QStringView     b) const noexcept;
    bool operator()(const QString & a, QLatin1String   b) const noexcept;

    bool operator()(QStringView     a, const QString & b) const noexcept;
    bool operator()(QLatin1String   a, const QString & b) const noexcept;

    bool operator()(QStringView     a, QLatin1String   b) const noexcept;
    bool operator()(QLatin1String   a, QStringView     b) const noexcept;

#if QT_VERSION >= QT_VERSION_CHECK(6, 5, 0)
    bool operator()(QUtf8StringView a, QUtf8StringView b) const noexcept;

    bool operator()(const QString & a, QUtf8StringView b) const noexcept;
    bool operator()(QStringView     a, QUtf8StringView b) const noexcept;
    bool operator()(QLatin1String   a, QUtf8StringView b) const noexcept;

    bool operator()(QUtf8StringView a, const QString & b) const noexcept;
    bool operator()(QUtf8StringView a, QStringView     b) const noexcept;
    bool operator()(QUtf8StringView a, QLatin1String   b) const noexcept;
#endif
    // clang-format on
};

/// Case insensitive transparent comparator for Qt's string types
using QCompareCaseInsensitive = QCompareTransparentBase<Qt::CaseInsensitive>;

/// Case sensitive transparent comparator for Qt's string types
using QCompareTransparent = QCompareTransparentBase<Qt::CaseSensitive>;

template <Qt::CaseSensitivity CS>
inline bool QCompareTransparentBase<CS>::operator()(
    const QString &a, const QString &b) const noexcept
{
    return a.compare(b, CS) < 0;
}

template <Qt::CaseSensitivity CS>
inline bool QCompareTransparentBase<CS>::operator()(
    QStringView a, QStringView b) const noexcept
{
    return a.compare(b, CS) < 0;
}

template <Qt::CaseSensitivity CS>
inline bool QCompareTransparentBase<CS>::operator()(
    QLatin1String a, QLatin1String b) const noexcept
{
    return a.compare(b, CS) < 0;
}

template <Qt::CaseSensitivity CS>
inline bool QCompareTransparentBase<CS>::operator()(
    const QString &a, QStringView b) const noexcept
{
    return a.compare(b, CS) < 0;
}

template <Qt::CaseSensitivity CS>
inline bool QCompareTransparentBase<CS>::operator()(
    const QString &a, QLatin1String b) const noexcept
{
    return a.compare(b, CS) < 0;
}

template <Qt::CaseSensitivity CS>
inline bool QCompareTransparentBase<CS>::operator()(
    QStringView a, const QString &b) const noexcept
{
    return a.compare(b, CS) < 0;
}

template <Qt::CaseSensitivity CS>
inline bool QCompareTransparentBase<CS>::operator()(
    QLatin1String a, const QString &b) const noexcept
{
    return a.compare(b, CS) < 0;
}

template <Qt::CaseSensitivity CS>
inline bool QCompareTransparentBase<CS>::operator()(
    QStringView a, QLatin1String b) const noexcept
{
    return a.compare(b, CS) < 0;
}

template <Qt::CaseSensitivity CS>
inline bool QCompareTransparentBase<CS>::operator()(
    QLatin1String a, QStringView b) const noexcept
{
    return a.compare(b, CS) < 0;
}

#if QT_VERSION >= QT_VERSION_CHECK(6, 5, 0)
template <Qt::CaseSensitivity CS>
inline bool QCompareTransparentBase<CS>::operator()(
    QUtf8StringView a, QUtf8StringView b) const noexcept
{
    return a.compare(b, CS) < 0;
}

template <Qt::CaseSensitivity CS>
inline bool QCompareTransparentBase<CS>::operator()(
    const QString &a, QUtf8StringView b) const noexcept
{
    return QStringView{a}.compare(b, CS) < 0;
}

template <Qt::CaseSensitivity CS>
inline bool QCompareTransparentBase<CS>::operator()(
    QStringView a, QUtf8StringView b) const noexcept
{
    return a.compare(b, CS) < 0;
}

template <Qt::CaseSensitivity CS>
inline bool QCompareTransparentBase<CS>::operator()(
    QLatin1String a, QUtf8StringView b) const noexcept
{
    return a.compare(b, CS) < 0;
}

template <Qt::CaseSensitivity CS>
inline bool QCompareTransparentBase<CS>::operator()(
    QUtf8StringView a, const QString &b) const noexcept
{
    return a.compare(b, CS) < 0;
}

template <Qt::CaseSensitivity CS>
inline bool QCompareTransparentBase<CS>::operator()(
    QUtf8StringView a, QStringView b) const noexcept
{
    return a.compare(b, CS) < 0;
}

template <Qt::CaseSensitivity CS>
inline bool QCompareTransparentBase<CS>::operator()(
    QUtf8StringView a, QLatin1String b) const noexcept
{
    return a.compare(b, CS) < 0;
}
#endif

}  // namespace chatterino
