#pragma once

#include "common/Channel.hpp"
#include "controllers/completion/sources/CommandSource.hpp"
#include "controllers/completion/sources/EmoteSource.hpp"
#include "controllers/completion/sources/Source.hpp"
#include "controllers/completion/sources/UserSource.hpp"

#include <functional>
#include <memory>

namespace chatterino::completion {

class UnifiedSource : public Source
{
public:
    /// @brief Initializes a unified completion source.
    /// @param sources Vector of sources to unify
    UnifiedSource(std::vector<std::unique_ptr<Source>> sources);

    void update(const QString &query) override;
    void addToListModel(GenericListModel &model,
                        size_t maxCount = 0) const override;
    void addToStringList(QStringList &list, size_t maxCount = 0,
                         bool isFirstWord = false) const override;

private:
    std::vector<std::unique_ptr<Source>> sources_;
};

}  // namespace chatterino::completion
