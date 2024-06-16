#pragma once

#include <string>

namespace chatterino {

struct PronounUser {
    std::string id;
    std::string username;
    PronounUser();
    PronounUser(std::string id, std::string username);
};

}  // namespace chatterino
