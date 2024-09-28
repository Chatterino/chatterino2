#pragma once

#include <QColor>
#include <QString>

namespace chatterino {
class Theme;

struct MessageColor {
    enum Type { Custom, Text, Link, System };

    MessageColor(const QColor &color);
    MessageColor(Type type_ = Text);

    const QColor &getColor(Theme &themeManager) const;

    QString toString() const;

private:
    Type type_;
    QColor customColor_;
};

}  // namespace chatterino
