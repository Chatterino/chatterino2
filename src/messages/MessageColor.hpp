#pragma once

#include <QColor>
#include <QString>

namespace chatterino {

struct MessageColors;

struct MessageColor {
    enum Type : uint8_t { Custom, Text, Link, System };

    MessageColor(const QColor &color);
    MessageColor(Type type_ = Text);

    Type type() const
    {
        return this->type_;
    }

    const QColor &getColor(const MessageColors &colors) const;

    QString toString() const;

private:
    Type type_;
    QColor customColor_;
};

}  // namespace chatterino
