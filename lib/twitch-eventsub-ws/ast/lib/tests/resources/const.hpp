#include <optional>
#include <vector>

struct Pod {
};

struct Const {
    const int a;
    const bool b;
    const char c;
    const Pod d;
    const std::vector<bool> e;
    const std::optional<bool> f;
};
