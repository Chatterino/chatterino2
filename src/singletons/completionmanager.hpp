#pragma once

#include <QAbstractListModel>
#include <QVector>
#include <map>
#include <string>

#include "helper/completionmodel.hpp"

namespace chatterino {
class CompletionManager
{
    CompletionManager() = default;

public:
    static CompletionManager &getInstance();

    CompletionModel *createModel(const std::string &channelName);

private:
    std::map<std::string, CompletionModel *> models;
};

}  // namespace chatterino
