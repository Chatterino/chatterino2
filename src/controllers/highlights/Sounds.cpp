// TODO: COPYRIGHT TEXT

#include "controllers/highlights/Sounds.hpp"

namespace chatterino::highlights {

const boost::container::flat_map<QString, DefaultSound> &defaultSounds()
{
    // If you add a new built-in sound here, double-check its licensing and attribute it
    // in the about page if necessary.
    static boost::container::flat_map<QString, DefaultSound> data{
        boost::container::ordered_unique_range_t{},
        {
            {
                "001-ping2",
                DefaultSound{
                    .id = "001-ping2",
                    .displayName = "Chatterino default",
                    .resourcePath = "qrc:/sounds/ping2.wav",
                },
            },
            {
                "002-sadiquecat-c4-harmonic",
                DefaultSound{
                    .id = "002-sadiquecat-c4-harmonic",
                    .displayName = "Sadiquecat C4 Harmonic",
                    .resourcePath = "qrc:/sounds/sadiquecat-c4-harmonic.wav",
                },
            },
            {
                "003-cat-fox_alex-fx-jump-3",
                DefaultSound{
                    .id = "003-cat-fox_alex-fx-jump-3",
                    .displayName = "CAT-FOX_Alex FX-Jump 3",
                    .resourcePath =
                        "qrc:/sounds/cat-fox_alex-8bit-fx-jump-3.wav",
                },
            },
        },
    };

    return data;
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
