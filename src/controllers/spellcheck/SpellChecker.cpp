#include "controllers/spellcheck/SpellChecker.hpp"

#include "Application.hpp"
#include "common/QLogging.hpp"
#include "controllers/accounts/AccountController.hpp"
#include "messages/Emote.hpp"
#include "providers/bttv/BttvEmotes.hpp"
#include "providers/seventv/SeventvEmotes.hpp"
#include "providers/twitch/TwitchAccount.hpp"
#include "providers/twitch/TwitchChannel.hpp"
#include "singletons/Paths.hpp"
#include "util/FilesystemHelpers.hpp"

#include <QTextCharFormat>

#ifdef CHATTERINO_WITH_SPELLCHECK
#    include <hunspell/hunspell.hxx>
#endif

namespace {

using namespace chatterino;

bool shouldIgnore(TwitchChannel *twitch, const QString &word)
{
    EmoteName name{word};
    if (twitch)
    {
        if (twitch->bttvEmote(name) || twitch->ffzEmote(name) ||
            twitch->seventvEmote(name))
        {
            return true;
        }
        auto locals = twitch->localTwitchEmotes();
        if (locals->contains(name))
        {
            return true;
        }

        if (twitch->accessChatters()->contains(word))
        {
            return true;
        }
    }
    if (getApp()->getBttvEmotes()->emote(name) ||
        getApp()->getFfzEmotes()->emote(name) ||
        getApp()->getSeventvEmotes()->globalEmote(name))
    {
        return true;
    }

    return getApp()
        ->getAccounts()
        ->twitch.getCurrent()
        ->twitchEmote(name)
        .has_value();
}

}  // namespace

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

void SpellChecker::reload()
{
#ifdef CHATTERINO_WITH_SPELLCHECK
    this->private_ = SpellCheckerPrivate::tryLoad();
#endif
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

SpellCheckHighlighter::SpellCheckHighlighter(QObject *parent)
    : QSyntaxHighlighter(parent)
    , wordRegex(R"(\p{L}+)",
                QRegularExpression::PatternOption::UseUnicodePropertiesOption)
{
    this->spellFmt.setUnderlineStyle(QTextCharFormat::SpellCheckUnderline);
    this->spellFmt.setUnderlineColor(Qt::red);
}

void SpellCheckHighlighter::setChannel(const std::shared_ptr<Channel> &channel)
{
    auto twitch = std::dynamic_pointer_cast<TwitchChannel>(channel);
    this->channel = twitch;
    this->rehighlight();
}

void SpellCheckHighlighter::highlightBlock(const QString &text)
{
    auto *spellChecker = getApp()->getSpellChecker();
    if (!spellChecker->isLoaded())
    {
        return;
    }
    auto *channel = this->channel.lock().get();

    QStringView textView = text;
#if QT_VERSION >= QT_VERSION_CHECK(6, 5, 0)
    auto it = this->wordRegex.globalMatchView(textView);
#else
    auto it = this->wordRegex.globalMatch(textView);
#endif
    while (it.hasNext())
    {
        auto match = it.next();
        auto text = match.captured();
        if (!shouldIgnore(channel, text) && !spellChecker->check(text))
        {
            this->setFormat(static_cast<int>(match.capturedStart()),
                            static_cast<int>(text.size()), this->spellFmt);
        }
    }
}

}  // namespace chatterino
