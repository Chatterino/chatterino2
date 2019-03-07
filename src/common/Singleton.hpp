#pragma once

#include <boost/noncopyable.hpp>

namespace chatterino
{
    class Settings;
    class Paths;

    class Singleton : boost::noncopyable
    {
    public:
        virtual ~Singleton() = default;

        virtual void initialize(Settings& settings, Paths& paths)
        {
            (void)(settings);
            (void)(paths);
        }

        virtual void save()
        {
        }
    };
}  // namespace chatterino
