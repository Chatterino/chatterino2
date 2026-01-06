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
    const QDir &searchDirectory)
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

        dictionaries.push_back(DictionaryInfo{
            .name = dictName,
            .path = searchDirectory.absoluteFilePath(dictName),
            .isSymbolicLink = isSymbolicLink,
        });
    }

    return dictionaries;
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
    std::filesystem::path aff;
    std::filesystem::path dic;
    if (path.isNull())
    {
        auto stdPath =
            qStringToStdPath(getApp()->getPaths().dictionariesDirectory);
        aff = stdPath / "index.aff";
        dic = stdPath / "index.dic";
    }
    else
    {
        aff = qStringToStdPath(path % ".aff");
        dic = qStringToStdPath(path % ".dic");
    }

    if (!std::filesystem::exists(aff) || !std::filesystem::exists(dic))
    {
        qCInfo(chatterinoSpellcheck)
            << "Failed to find index.aff or index.dic in 'Dictionaries'";
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
    : private_(SpellCheckerPrivate::tryLoad())
{
    // The method we load dictionaries by is bound to change, so it's OK for this to be a bit ugly as we test things.

    if (!this->private_)
    {
        // The default dictionary was not found, try the fallback if it's set
        auto fallbackDictionary =
            getSettings()->spellCheckingFallback.getValue();
        if (!fallbackDictionary.isEmpty())
        {
            this->private_ = SpellCheckerPrivate::tryLoad(fallbackDictionary);
        }
    }
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
std::vector<DictionaryInfo> SpellChecker::getSystemDictionaries() const
{
#ifdef CHATTERINO_WITH_SPELLCHECK
#    if defined(Q_OS_UNIX) and !defined(Q_OS_DARWIN)
    // For each XDG data directory, search in hunspell, myspell, and myspell/dicts.
    // This somewhat matches where dictionaries are stored on Ubuntu & Fedora
    // if we want to support defaulting to your LC_ALL language.

    QStringList searchDirectories;
    auto dataDirs = getXDGBaseDirectories(XDGDirectoryType::Data);
    for (const auto &dataDir : dataDirs)
    {
        searchDirectories.push_back(combinePath(dataDir, "hunspell"));
        searchDirectories.push_back(combinePath(dataDir, "myspell"));
        searchDirectories.push_back(combinePath(dataDir, "myspell/dicts"));
    }

    std::vector<DictionaryInfo> dictionaries;

    for (const auto &searchDirectory : searchDirectories)
    {
        qCDebug(chatterinoSpellcheck)
            << "Looking for system dictionaries in" << searchDirectory;
        for (const auto &dict : loadDictionariesFromDirectory(searchDirectory))
        {
            if (dict.isSymbolicLink)
            {
                // NOTE: We currently filter out symbolic links from system-loaded dictionaries.
                // Without this, the list of dictionaries we "support" would be too high on Linux distros.
                // As an example, this is the symlinks the installation of `hunspell-en-gb` creates on Arch Linux:
                // - en_AG.aff -> en_GB-large.aff
                // - en_BS.aff -> en_GB-large.aff
                // - en_BW.aff -> en_GB-large.aff
                // - en_BZ.aff -> en_GB-large.aff
                // - en_DK.aff -> en_GB-large.aff
                // - en_GB.aff -> en_GB-large.aff
                // - en_GB-large.aff
                // - en_GH.aff -> en_GB-large.aff
                // - en_HK.aff -> en_GB-large.aff
                // - en_IE.aff -> en_GB-large.aff
                // - en_IN.aff -> en_GB-large.aff
                // - en_JM.aff -> en_GB-large.aff
                // - en_NA.aff -> en_GB-large.aff
                // - en_NG.aff -> en_GB-large.aff
                // - en_NZ.aff -> en_GB-large.aff
                // - en_SG.aff -> en_GB-large.aff
                // - en_TT.aff -> en_GB-large.aff
                // - en_ZA.aff -> en_GB-large.aff
                // - en_ZW.aff -> en_GB-large.aff
                continue;
            }

            dictionaries.push_back(DictionaryInfo{
                .name = dict.name % " (System)",
                .path = dict.path,
                .isSymbolicLink = false,
            });
        }
    }

    return dictionaries;
#    else
    return {};
#    endif
#else
    return {};
#endif
}

}  // namespace chatterino
