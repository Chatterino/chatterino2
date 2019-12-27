#pragma once

namespace chatterino {

enum class ColorType { SelfHighlight, Subscription, Whisper };

class ColorProvider
{
public:
    static const ColorProvider &instance();

    const std::shared_ptr<QColor> color(ColorType type) const;

    void updateColor(ColorType type, QColor color);

    QSet<QColor> recentColors() const;

    const std::vector<QColor> &defaultColors() const;

private:
    ColorProvider();

    void initTypeColorMap();
    void initDefaultColors();

    std::unordered_map<ColorType, std::shared_ptr<QColor>> typeColorMap_;
    std::vector<QColor> defaultColors_;
};
}  // namespace chatterino

// Adapted from Qt example: https://doc.qt.io/qt-5/qhash.html#qhash
inline uint qHash(const QColor &key)
{
    return qHash(key.name(QColor::HexArgb));
}
