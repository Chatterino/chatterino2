#pragma once

#include <QStringList>
#include <QStringView>

namespace chatterino {

// Splits the string command into a list of tokens, and returns the list.
// Tokens with spaces can be surrounded by double quotes;
// three consecutive double quotes represent the quote character itself.
//
// Backported from QProcess 5.15
QStringList splitCommand(QStringView command);

}  // namespace chatterino
