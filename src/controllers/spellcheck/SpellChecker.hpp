#pragma once

#include <QString>

#include <memory>
#include <string>
#include <vector>

namespace chatterino {

class Channel;
class TwitchChannel;

class SpellCheckerPrivate;
class SpellChecker
{
public:
    SpellChecker();
    ~SpellChecker();

    bool isLoaded() const;

    bool check(const QString &word);
    std::vector<std::string> suggestions(const QString &word);

private:
    std::unique_ptr<SpellCheckerPrivate> private_;
};

}  // namespace chatterino
