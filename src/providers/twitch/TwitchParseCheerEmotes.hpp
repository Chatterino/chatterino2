#pragma once

#include <rapidjson/document.h>
#include <QString>
#include <map>
#include <vector>
#include "messages/Image.hpp"

namespace chatterino {

struct JSONCheermoteSet {
    QString prefix;
    std::vector<QString> scales;

    std::vector<QString> backgrounds;
    std::vector<QString> states;

    QString type;
    QString updatedAt;
    int priority;

    struct CheermoteTier {
        int minBits;
        QString id;
        QString color;

        //       Background        State             Scale
        std::map<QString, std::map<QString, std::map<QString, ImagePtr>>>
            images;
    };

    std::vector<CheermoteTier> tiers;
};

std::vector<JSONCheermoteSet> ParseCheermoteSets(const rapidjson::Document &d);

}  // namespace chatterino
