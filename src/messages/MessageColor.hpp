#pragma once

#include <QColor>

namespace chatterino
{
    class Theme;

    struct MessageColor
    {
        enum Type { Custom, Text, Link, System };

        MessageColor(const QColor& color);
        MessageColor(Type type_ = Text);

        const QColor& getColor(Theme& themeManager) const;

    private:
        Type type_;
        QColor customColor_;
    };

}  // namespace chatterino
