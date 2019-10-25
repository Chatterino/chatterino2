#pragma once

namespace chatterino {

enum class ColorType { SelfHighlight, Subscription, Whisper };

class ColorProvider
{
public:
    static const ColorProvider &instance();

    const std::shared_ptr<QColor> color(ColorType type) const;

    void updateColor(ColorType type, QColor color);

private:
    ColorProvider();

    std::unordered_map<ColorType, std::shared_ptr<QColor>> typeColorMap;
};

}  // namespace chatterino
