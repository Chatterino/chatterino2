#include <optional>
#include <string>
#include <vector>

struct String {
    std::string a;
    const std::string b;
    std::vector<std::string> c;
    std::optional<std::string> d;
};
