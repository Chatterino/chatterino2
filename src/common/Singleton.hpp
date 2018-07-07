#pragma once

#include <boost/noncopyable.hpp>

namespace chatterino {

class Application;

class Singleton : boost::noncopyable
{
public:
    virtual void initialize(Application &app)
    {
        (void)(app);
    }

    virtual void save()
    {
    }
};

}  // namespace chatterino
