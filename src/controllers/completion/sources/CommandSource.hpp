#pragma once

#include "controllers/completion/sources/Source.hpp"
#include "controllers/completion/strategies/Strategy.hpp"

#include <QString>

#include <functional>
#include <memory>
#include <vector>

namespace chatterino::completion {

struct CommandItem {
    QString name{};
    QString prefix{};
};

class CommandSource : public Source
{
public:
    using ActionCallback = std::function<void(const QString &)>;
    using CommandStrategy = Strategy<CommandItem>;

    /// @brief Initializes a source for CommandItems.
    /// @param strategy Strategy to apply
    /// @param callback ActionCallback to invoke upon InputCompletionItem selection.
    /// See InputCompletionItem::action(). Can be nullptr.
    CommandSource(std::unique_ptr<CommandStrategy> strategy,
                  ActionCallback callback = nullptr);

    void update(const QString &query) override;
    void addToListModel(GenericListModel &model,
                        size_t maxCount = 0) const override;
    void addToStringList(QStringList &list, size_t maxCount = 0,
                         bool isFirstWord = false) const override;

    const std::vector<CommandItem> &output() const;

private:
    void initializeItems();

    std::unique_ptr<CommandStrategy> strategy_;
    ActionCallback callback_;

    std::vector<CommandItem> items_{};
    std::vector<CommandItem> output_{};
};

}  // namespace chatterino::completion
