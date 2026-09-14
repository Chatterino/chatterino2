// SPDX-FileCopyrightText: 2026 Contributors to Chatterino <https://chatterino.com>
//
// SPDX-License-Identifier: MIT

#include "controllers/hotkeys/HotkeySequence.hpp"

#include <QLatin1String>
#include <QString>
#include <QStringView>

namespace {

constexpr int MIN_EXTRA_BUTTON_NUMBER = 4;
constexpr int MAX_EXTRA_BUTTON_NUMBER = 27;  // Qt::ExtraButton24

}  // namespace

namespace chatterino {

std::optional<int> extraMouseButtonNumber(Qt::MouseButton button)
{
    if (button == Qt::NoButton)
    {
        return std::nullopt;
    }

    auto value = static_cast<unsigned>(button);
    // Must be a single bit (one button, not a mask).
    if ((value & (value - 1)) != 0)
    {
        return std::nullopt;
    }

    int number = 1;
    while (value > 1)
    {
        value >>= 1;
        number++;
    }

    if (number < MIN_EXTRA_BUTTON_NUMBER || number > MAX_EXTRA_BUTTON_NUMBER)
    {
        return std::nullopt;
    }

    return number;
}

bool isExtraMouseButton(Qt::MouseButton button)
{
    return extraMouseButtonNumber(button).has_value();
}

std::optional<Qt::MouseButton> extraMouseButtonFromNumber(int number)
{
    if (number < MIN_EXTRA_BUTTON_NUMBER || number > MAX_EXTRA_BUTTON_NUMBER)
    {
        return std::nullopt;
    }

    return static_cast<Qt::MouseButton>(1u << (number - 1));
}

namespace {

std::optional<Qt::MouseButton> parseMouseButton(const QString &text)
{
    if (text.compare(u"MouseBack", Qt::CaseInsensitive) == 0)
    {
        return Qt::BackButton;
    }
    if (text.compare(u"MouseForward", Qt::CaseInsensitive) == 0)
    {
        return Qt::ForwardButton;
    }

    constexpr auto PREFIX = QLatin1String("MouseButton");
    if (!text.startsWith(PREFIX, Qt::CaseInsensitive))
    {
        return std::nullopt;
    }

    bool ok = false;
    int number = QStringView(text).sliced(PREFIX.size()).toInt(&ok);
    if (!ok)
    {
        return std::nullopt;
    }

    return extraMouseButtonFromNumber(number);
}

bool looksLikeMouseBinding(const QString &text)
{
    return text.compare(u"MouseBack", Qt::CaseInsensitive) == 0 ||
           text.compare(u"MouseForward", Qt::CaseInsensitive) == 0 ||
           text.startsWith(QLatin1String("MouseButton"), Qt::CaseInsensitive);
}

QString extraMouseButtonName(Qt::MouseButton button, bool portable)
{
    if (button == Qt::BackButton)
    {
        return portable ? QStringLiteral("MouseBack")
                        : QStringLiteral("Mouse Back");
    }
    if (button == Qt::ForwardButton)
    {
        return portable ? QStringLiteral("MouseForward")
                        : QStringLiteral("Mouse Forward");
    }

    auto number = extraMouseButtonNumber(button);
    if (!number)
    {
        return {};
    }

    return portable ? QStringLiteral("MouseButton%1").arg(*number)
                    : QStringLiteral("Mouse Button %1").arg(*number);
}

}  // namespace

HotkeySequence::HotkeySequence(const QKeySequence &keySequence)
    : keySequence_(keySequence)
{
}

HotkeySequence::HotkeySequence(Qt::MouseButton mouseButton)
    : mouseButton_(isExtraMouseButton(mouseButton) ? mouseButton : Qt::NoButton)
{
}

HotkeySequence HotkeySequence::fromPortableString(const QString &text)
{
    auto trimmed = text.trimmed();
    if (trimmed.isEmpty())
    {
        return {};
    }

    if (looksLikeMouseBinding(trimmed))
    {
        if (auto button = parseMouseButton(trimmed))
        {
            return HotkeySequence(*button);
        }
        return {};
    }

    return {QKeySequence(trimmed, QKeySequence::PortableText)};
}

bool HotkeySequence::isEmpty() const
{
    return this->mouseButton_ == Qt::NoButton && this->keySequence_.isEmpty();
}

bool HotkeySequence::isMouse() const
{
    return this->mouseButton_ != Qt::NoButton;
}

QKeySequence HotkeySequence::keySequence() const
{
    return this->keySequence_;
}

Qt::MouseButton HotkeySequence::mouseButton() const
{
    return this->mouseButton_;
}

QString HotkeySequence::toString() const
{
    if (this->isMouse())
    {
        return extraMouseButtonName(this->mouseButton_, false);
    }

    return this->keySequence_.toString(QKeySequence::NativeText);
}

QString HotkeySequence::toPortableString() const
{
    if (this->isMouse())
    {
        return extraMouseButtonName(this->mouseButton_, true);
    }

    return this->keySequence_.toString(QKeySequence::PortableText);
}

bool HotkeySequence::operator==(const HotkeySequence &other) const
{
    return this->mouseButton_ == other.mouseButton_ &&
           this->keySequence_ == other.keySequence_;
}

bool HotkeySequence::operator!=(const HotkeySequence &other) const
{
    return !(*this == other);
}

}  // namespace chatterino
