#pragma once

#include <QAbstractListModel>
#include <QVector>
#include <map>
#include <string>

#include "helper/completionmodel.hpp"

namespace chatterino {
namespace singletons {
class CompletionManager
{
    CompletionManager() = default;

public:
    static CompletionManager &getInstance();

    CompletionModel *createModel(const QString &channelName);

private:
    std::map<QString, CompletionModel *> models;
};

}  // namespace singletons
}  // namespace chatterino
