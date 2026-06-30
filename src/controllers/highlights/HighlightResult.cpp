// SPDX-FileCopyrightText: 2025 Contributors to Chatterino <https://chatterino.com>
//
// SPDX-License-Identifier: MIT

#include "controllers/highlights/HighlightResult.hpp"

#include <qdebug.h>

namespace chatterino {

HighlightResult HighlightResult::emptyResult()
{
    return {
        .ids = {},
        .alert = false,
        .playSound = false,
        .customSoundUrl = std::nullopt,
        .color = nullptr,
        .showInMentions = false,
    };
}

bool HighlightResult::operator==(const HighlightResult &other) const
{
    if (this->alert != other.alert)
    {
        qInfo() << "did not match alert";
        return false;
    }
    if (this->playSound != other.playSound)
    {
        qInfo() << "did not match playSound";
        return false;
    }
    if (auto ourUrl = this->customSoundUrl.value_or(QUrl{}),
        theirUrl = other.customSoundUrl.value_or(QUrl{});
        ourUrl != theirUrl)
    {
        qInfo() << "did not match customSoundUrl";
        return false;
    }

    if (this->color && other.color)
    {
        if (*this->color != *other.color)
        {
            qInfo() << "did not match color";
            return false;
        }
    }

    if (this->showInMentions != other.showInMentions)
    {
        qInfo() << "did not match show in mentions";
        return false;
    }

    return true;
}

bool HighlightResult::operator!=(const HighlightResult &other) const
{
    return !(*this == other);
}

bool HighlightResult::empty() const
{
    return !this->alert && !this->playSound &&
           !this->customSoundUrl.has_value() && !this->color &&
           !this->showInMentions;
}

bool HighlightResult::full() const
{
    return this->alert && this->playSound && this->customSoundUrl.has_value() &&
           this->color && this->showInMentions;
}

std::ostream &operator<<(std::ostream &os, const HighlightResult &result)
{
    os << "IDs: " << result.ids.join(',').toStdString()
       << ", Alert: " << (result.alert ? "Yes" : "No") << ", "
       << "Play sound: " << (result.playSound ? "Yes" : "No") << " ("
       << (result.customSoundUrl
               ? result.customSoundUrl->toString().toStdString()
               : "")
       << ")"
       << ", "
       << "Color: "
       << (result.color
               ? result.color->name(QColor::NameFormat::HexArgb).toStdString()
               : "")
       << ", "
       << "Show in mentions: " << (result.showInMentions ? "Yes" : "No");
    return os;
}

}  // namespace chatterino
