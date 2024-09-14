#pragma once

#include <QColor>

namespace chatterino {
class Theme;

struct MessageColor {
    enum Type : uint8_t { Custom, Text, Link, System };

    MessageColor(const QColor &color);
    MessageColor(Type type_ = Text);

    Type type() const
    {
        return this->type_;
    }

    const QColor &getColor(Theme &themeManager) const;

private:
    Type type_;
    QColor customColor_;
};

}  // namespace chatterino
