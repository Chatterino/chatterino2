#pragma once

#include <QColor>

#include <memory>
#include <unordered_map>
#include <vector>

namespace chatterino {

enum class ColorType {
    SelfHighlight,
    Subscription,
    Whisper,
    RedeemedHighlight,
    FirstMessageHighlight,
    ElevatedMessageHighlight,
    ThreadMessageHighlight,
    // Used in automatic highlights of your own messages
    SelfMessageHighlight,
    AutomodHighlight,
};

class ColorProvider
{
public:
    static const ColorProvider &instance();

    /**
     * @brief Return a std::shared_ptr to the color of the requested ColorType.
     *
     * If a custom color has been set for the requested ColorType, it is
     * returned. If no custom color exists for the type, a default color is
     * returned.
     *
     * We need to do this in order to be able to dynamically update the colors
     * of already parsed predefined (self highlights, subscriptions,
     * and whispers) highlights.
     */
    std::shared_ptr<QColor> color(ColorType type) const;

    /**
     * @brief Return a set of recently used colors used anywhere in Chatterino.
     */
    QSet<QColor> recentColors() const;

    /**
     * @brief Return a vector of colors that are good defaults for use
     *        throughout the program.
     */
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
