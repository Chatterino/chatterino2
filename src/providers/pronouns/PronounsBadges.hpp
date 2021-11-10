#pragma once

#include <boost/optional.hpp>
#include <common/Singleton.hpp>

#include "common/Aliases.hpp"
#include "util/QStringHash.hpp"

#include <map>
#include <memory>
#include <shared_mutex>
#include <vector>

namespace chatterino {

class PronounsBadges : public Singleton
{
public:
    virtual void initialize(Settings &settings, Paths &paths) override;
    PronounsBadges() = default;

    boost::optional<QString> getPronouns(const UserId &id,
                                         const QString &userName);

private:
    void loadPronouns();

    std::shared_mutex mutex_;

    std::unordered_map<QString, boost::optional<QString>> userPronounsMap;
    std::unordered_map<QString, QString> pronounsMap;
};

}  // namespace chatterino
