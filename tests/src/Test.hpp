#pragma once

#include <gmock/gmock.h>  // IWYU pragma: export
#include <gtest/gtest.h>  // IWYU pragma: export

#include <ostream>

class QString;
class QStringView;
class QByteArray;

// This file is included in all TUs in chatterino-test to avoid ODR violations.
std::ostream &operator<<(std::ostream &os, QStringView str);
std::ostream &operator<<(std::ostream &os, const QByteArray &bytes);
std::ostream &operator<<(std::ostream &os, const QString &str);

// NOLINTBEGIN(readability-identifier-naming)
// PrintTo is used for naming parameterized tests, and is part of gtest
void PrintTo(const QByteArray &bytes, std::ostream *os);
void PrintTo(QStringView str, std::ostream *os);
void PrintTo(const QString &str, std::ostream *os);
// NOLINTEND(readability-identifier-naming)
