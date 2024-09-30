#pragma once

#include <QColor>
#include <QString>

namespace chatterino {

struct MessageColors;

struct MessageColor {
    enum Type { Custom, Text, Link, System };

    MessageColor(const QColor &color);
    MessageColor(Type type_ = Text);

    const QColor &getColor(const MessageColors &colors) const;

    QString toString() const;

private:
    Type type_;
    QColor customColor_;
};

}  // namespace chatterino
