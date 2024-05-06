#include "Test.hpp"

#include <QString>
#include <QStringView>

std::ostream &operator<<(std::ostream &os, QStringView str)
{
    os << str.toString().toStdString();
    return os;
}

std::ostream &operator<<(std::ostream &os, const QByteArray &bytes)
{
    os << std::string_view{bytes.data(), static_cast<size_t>(bytes.size())};
    return os;
}

std::ostream &operator<<(std::ostream &os, const QString &str)
{
    os << str.toStdString();
    return os;
}

// The PrintTo overloads use UniversalPrint to print strings in quotes.
// Even though this uses testing::internal, this is publically documented in
// gtest/gtest-printers.h.

void PrintTo(const QByteArray &bytes, std::ostream *os)
{
    ::testing::internal::UniversalPrint(bytes.toStdString(), os);
}

void PrintTo(QStringView str, std::ostream *os)
{
    ::testing::internal::UniversalPrint(
        std::u16string{str.utf16(), static_cast<size_t>(str.size())}, os);
}

void PrintTo(const QString &str, std::ostream *os)
{
    ::testing::internal::UniversalPrint(str.toStdU16String(), os);
}
