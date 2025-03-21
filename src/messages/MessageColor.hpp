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

    bool operator==(const MessageColor &other) const noexcept
    {
        return this->type_ == other.type_ &&
               this->customColor_ == other.customColor_;
    }

private:
    Type type_;
    QColor customColor_;
};

}  // namespace chatterino
