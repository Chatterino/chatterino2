// SPDX-FileCopyrightText: 2026 Contributors to Chatterino <https://chatterino.com>
//
// SPDX-License-Identifier: MIT

#pragma once

#include <QString>

#include <memory>
#include <string>
#include <vector>

namespace chatterino {

class Channel;
class TwitchChannel;

struct DictionaryInfo {
    /// The name of the dictionary to be shown to users (e.g. "en_GB (System)")
    QString name;

    /// The absolute path to the dictionary without the .aff or .dic suffix (e.g. "/foo/bar/en_GB")
    QString path;

    bool isSymbolicLink;
};

class SpellCheckerPrivate;
class SpellChecker
{
public:
    SpellChecker();
    ~SpellChecker();

    bool isLoaded() const;

    bool check(const QString &word);
    std::vector<std::string> suggestions(const QString &word);

    /// Return a list of system-installed dictionaries.
    ///
    /// Currently only implemented on Linux.
    std::vector<DictionaryInfo> getSystemDictionaries() const;

private:
    std::unique_ptr<SpellCheckerPrivate> private_;
};

}  // namespace chatterino
