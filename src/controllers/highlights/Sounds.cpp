// TODO: COPYRIGHT TEXT

#include "controllers/highlights/Sounds.hpp"

namespace chatterino::highlights {

const std::map<QString, DefaultSound> &defaultSounds()
{
    static std::map<QString, DefaultSound> defaultSoundsXD{
        {
            "ping2",
            DefaultSound{
                .id = "ping2",
                .displayName = "Chatterino default",
                .resourcePath = "qrc:/sounds/ping2.wav",
            },
        },
        {
            "ping3",
            DefaultSound{
                .id = "ping3",
                .displayName = "Chatterino laser",
                .resourcePath = "qrc:/sounds/ping3.wav",
            },
        },
    };

    return defaultSoundsXD;
}

std::optional<DefaultSound> resolveDefaultSound(QStringView id)
{
    const auto &m = defaultSounds();

    auto it = m.find(id.toString());
    if (it == m.end())
    {
        return std::nullopt;
    }

    return it->second;
}

}  // namespace chatterino::highlights
