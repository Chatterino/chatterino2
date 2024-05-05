#pragma once

// Include guard for some compilers not supporting `#pragma once` for prepended
// includes.
#ifndef TEST_HELPERS_HPP_INCLUDED
#    define TEST_HELPERS_HPP_INCLUDED

#    include <ostream>

// This file is included in all TUs in chatterino-test to avoid ODR violations.
//
// It defines overloads for printing Qt types to a std::ostream as well as
// overloads for PrintTo which is used for naming parameterized tests.

class QString;
class QStringView;
class QByteArray;

std::ostream &operator<<(std::ostream &os, QStringView str);
std::ostream &operator<<(std::ostream &os, const QByteArray &bytes);
std::ostream &operator<<(std::ostream &os, const QString &str);

// NOLINTBEGIN(readability-identifier-naming)
void PrintTo(const QByteArray &bytes, std::ostream *os);
void PrintTo(QStringView str, std::ostream *os);
void PrintTo(const QString &str, std::ostream *os);
// NOLINTEND(readability-identifier-naming)

#endif
