// This is a header used for testing if the library can parse a file.

#include <boost/json.hpp>
#include <QString>

#include <chrono>

boost::json::value doSomething(QString s,
                               std::chrono::system_clock::time_point tp);

static_assert(sizeof(QString) > sizeof(int));
static_assert(sizeof(boost::json::value) > sizeof(int));
