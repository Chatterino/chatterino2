#pragma once

#include <boost/noncopyable.hpp>

namespace chatterino {

class Application;

class Singleton : boost::noncopyable
{
    virtual ~Singleton() = default;

    virtual void initialize(Application &application)
    {
        (void)(application);
    }

    virtual void finalize()
    {
    }
};

}  // namespace chatterino
