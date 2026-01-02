#include "controllers/spellcheck/SpellChecker.hpp"

#include "Application.hpp"
#include "common/QLogging.hpp"
#include "singletons/Paths.hpp"
#include "util/FilesystemHelpers.hpp"

#ifdef CHATTERINO_WITH_SPELLCHECK
#    include <hunspell/hunspell.hxx>
#endif

namespace chatterino {

#ifdef CHATTERINO_WITH_SPELLCHECK
class SpellCheckerPrivate
{
public:
    static std::unique_ptr<SpellCheckerPrivate> tryLoad();

    Hunspell hunspell;

private:
    SpellCheckerPrivate(const char *affpath, const char *dpath);
};

std::unique_ptr<SpellCheckerPrivate> SpellCheckerPrivate::tryLoad()
{
    auto stdPath = qStringToStdPath(getApp()->getPaths().dictionariesDirectory);
    auto aff = stdPath / "index.aff";
    auto dic = stdPath / "index.dic";
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

}  // namespace chatterino
