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

    /// The path to the dictionary without the .aff or .dic suffix (e.g. "/foo/bar/en_GB" or "en_GB")
    ///
    /// Paths must be absolute if they're marked as a system dictionary,
    /// otherwise they must be relative to the Chatterino Dictionaries
    /// directory.
    QString path;

    bool isSymbolicLink;
    bool isSystem;
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

    /// Get a list of dictionaries from the Chatterino Dictionaries directory
    /// and the system directories if supported.
    ///
    /// System-dictionary loading is currently only implemented on Linux.
    std::vector<DictionaryInfo> getAvailableDictionaries() const;

private:
    std::unique_ptr<SpellCheckerPrivate> private_;
};

}  // namespace chatterino
