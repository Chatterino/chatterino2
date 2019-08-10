#pragma once

#include <QString>
#include <boost/optional.hpp>

namespace chatterino {

struct Emote;
using EmotePtr = std::shared_ptr<const Emote>;

class FfzModBadge
{
public:
    FfzModBadge(const QString &channelName);

    void loadCustomModBadge();

    EmotePtr badge() const;

private:
    const QString channelName_;
    EmotePtr badge_;
};

}  // namespace chatterino
