// SPDX-FileCopyrightText: 2026 Contributors to Chatterino <https://chatterino.com>
//
// SPDX-License-Identifier: MIT

#include "controllers/spellcheck/SpellChecker.hpp"

#include "Application.hpp"
#include "common/QLogging.hpp"
#include "singletons/Paths.hpp"
#include "singletons/Settings.hpp"
#include "util/CombinePath.hpp"
#include "util/FilesystemHelpers.hpp"
#include "util/XDGDirectory.hpp"

#ifdef CHATTERINO_WITH_SPELLCHECK
#    include <hunspell/hunspell.hxx>
#endif

namespace chatterino {

#ifdef CHATTERINO_WITH_SPELLCHECK

namespace {

/// Returns a list of available dictionaries in the given directory
std::vector<DictionaryInfo> loadDictionariesFromDirectory(
    const QDir &searchDirectory, bool isSystem)
{
    std::vector<DictionaryInfo> dictionaries;

    for (const auto &affInfo : searchDirectory.entryInfoList(
             {"*.aff"}, QDir::Files | QDir::NoDotAndDotDot, QDir::Name))
    {
        if (!affInfo.isFile())
        {
            continue;
        }

        auto dictName = affInfo.baseName();

        auto dicInfo = QFileInfo(searchDirectory, dictName % ".dic");
        if (!dicInfo.isFile())
        {
            continue;
        }

        auto isSymbolicLink =
            affInfo.isSymbolicLink() || dicInfo.isSymbolicLink();

        QString path = [&] {
            if (isSystem)
            {
                return searchDirectory.absoluteFilePath(dictName);
            }
            return dictName;
        }();

        dictionaries.push_back(DictionaryInfo{
            .name = dictName,
            .path = path,
            .isSymbolicLink = isSymbolicLink,
            .isSystem = isSystem,
        });
    }

    return dictionaries;
}

QString resolveDictionaryPath(const QString &path)
{
    if (QDir::isAbsolutePath(path))
    {
        return path;
    }
    return combinePath(getApp()->getPaths().dictionariesDirectory, path);
}

}  // namespace

class SpellCheckerPrivate
{
public:
    static std::unique_ptr<SpellCheckerPrivate> tryLoad(
        const QString &path = {});

    /// NOTE: To support multiple dictionaries at the same time, it seems like we need to store a list of Hunspell instances, each supporting a single dictionary, and then during the spell checking process check each hunspell instance.
    Hunspell hunspell;

private:
    SpellCheckerPrivate(const char *affpath, const char *dpath);
};

std::unique_ptr<SpellCheckerPrivate> SpellCheckerPrivate::tryLoad(
    const QString &path)
{
    if (path.isEmpty())
    {
        qCDebug(chatterinoSpellcheck) << "No path specified";
        return nullptr;
    }

    auto resolvedPath = resolveDictionaryPath(path);
    auto aff = qStringToStdPath(resolvedPath % ".aff");
    auto dic = qStringToStdPath(resolvedPath % ".dic");

    if (!std::filesystem::exists(aff) || !std::filesystem::exists(dic))
    {
        qCInfo(chatterinoSpellcheck).nospace().noquote()
            << "Failed to find " << resolvedPath << ".{aff,dic}";
        return nullptr;
    }
    std::error_code ec;
    auto affCanonical = std::filesystem::weakly_canonical(aff, ec);
    if (ec)
    {
        qCInfo(chatterinoSpellcheck)
            << "Failed to canonicalize" << stdPathToQString(aff)
            << "error:" << QUtf8StringView(ec.message());
        return nullptr;
    }
    auto dicCanonical = std::filesystem::weakly_canonical(dic, ec);
    if (ec)
    {
        qCInfo(chatterinoSpellcheck)
            << "Failed to canonicalize" << stdPathToQString(dic)
            << "error:" << QUtf8StringView(ec.message());
        return nullptr;
    }

    return std::unique_ptr<SpellCheckerPrivate>{new SpellCheckerPrivate(
        affCanonical.string().c_str(), dicCanonical.string().c_str())};
}

SpellCheckerPrivate::SpellCheckerPrivate(const char *affpath, const char *dpath)
    : hunspell(affpath, dpath)
{
}

SpellChecker::SpellChecker()
    : private_(SpellCheckerPrivate::tryLoad(
          getSettings()->spellCheckingDefaultDictionary))
{
}
#else
class SpellCheckerPrivate
{
};
SpellChecker::SpellChecker() = default;
#endif

SpellChecker::~SpellChecker() = default;

bool SpellChecker::isLoaded() const
{
    return this->private_ != nullptr;
}

// NOLINTNEXTLINE(readability-convert-member-functions-to-static)
bool SpellChecker::check(const QString &word)
{
#ifdef CHATTERINO_WITH_SPELLCHECK
    if (!this->private_)
    {
        return true;
    }

    return this->private_->hunspell.spell(word.toStdString());
#else
    (void)word;
    return true;
#endif
}

// NOLINTNEXTLINE(readability-convert-member-functions-to-static)
std::vector<std::string> SpellChecker::suggestions(const QString &word)
{
#ifdef CHATTERINO_WITH_SPELLCHECK
    if (!this->private_)
    {
        return {};
    }

    auto stdWord = word.toStdString();
    if (this->private_->hunspell.spell(stdWord))
    {
        return {};
    }

    return this->private_->hunspell.suggest(stdWord);
#else
    (void)word;
    return {};
#endif
}

// NOLINTNEXTLINE(readability-convert-member-functions-to-static)
std::vector<DictionaryInfo> SpellChecker::getAvailableDictionaries() const
{
#ifdef CHATTERINO_WITH_SPELLCHECK
    std::vector<std::pair<QString, bool>> searchDirectories{
        {getApp()->getPaths().dictionariesDirectory, false},
    };

#    if defined(Q_OS_UNIX) and !defined(Q_OS_DARWIN)
    // For each XDG data directory, search in hunspell, myspell, and myspell/dicts.
    // This somewhat matches where dictionaries are stored on Ubuntu & Fedora
    // if we want to support defaulting to your LC_ALL language.

    auto dataDirs = getXDGBaseDirectories(XDGDirectoryType::Data);
    for (const auto &dataDir : dataDirs)
    {
        searchDirectories.emplace_back(combinePath(dataDir, "hunspell"), true);
        searchDirectories.emplace_back(combinePath(dataDir, "myspell"), true);
        searchDirectories.emplace_back(combinePath(dataDir, "myspell/dicts"),
                                       true);
    }
#    endif

    std::vector<DictionaryInfo> dictionaries;

    for (const auto &[searchDirectory, isSystem] : searchDirectories)
    {
        qCDebug(chatterinoSpellcheck)
            << "Looking for dictionaries in" << searchDirectory
            << "isSystem:" << isSystem;
        for (const auto &dict :
             loadDictionariesFromDirectory(searchDirectory, isSystem))
        {
            if (dict.isSymbolicLink && dict.isSystem)
            {
                // NOTE: We currently filter out symbolic links from system-loaded dictionaries.
                // Without this, the list of dictionaries we "support" would be too high on Linux distros.
                // As an example, this is the symlinks the installation of `hunspell-en-gb` creates on Arch Linux:
                // - en_AG.aff -> en_GB-large.aff
                // -  ...
                // - en_GB-large.aff
                continue;
            }
            auto name = [&] -> QString {
                if (dict.isSystem)
                {
                    return dict.name % " (System)";
                }
                return dict.name;
            }();

            dictionaries.push_back(DictionaryInfo{
                .name = name,
                .path = dict.path,
                .isSymbolicLink = dict.isSymbolicLink,
                .isSystem = dict.isSystem,
            });
        }
    }

    std::ranges::sort(dictionaries,
                      [](const DictionaryInfo &lhs, const DictionaryInfo &rhs) {
                          return std::tie(lhs.isSystem, lhs.name, lhs.path) <
                                 std::tie(rhs.isSystem, rhs.name, rhs.path);
                      });

    return dictionaries;
#else
    return {};
#endif
}

}  // namespace chatterino
