#pragma once

namespace chatterino {

class Settings;
class Paths;

class Singleton
{
public:
    Singleton() = default;
    virtual ~Singleton() = default;

    Singleton(const Singleton &) = delete;
    Singleton &operator=(const Singleton &) = delete;

    Singleton(Singleton &&) = delete;
    Singleton &operator=(Singleton &&) = delete;

    virtual void initialize(Settings &settings, Paths &paths)
    {
        (void)(settings);
        (void)(paths);
    }

    virtual void save()
    {
    }
};

}  // namespace chatterino
