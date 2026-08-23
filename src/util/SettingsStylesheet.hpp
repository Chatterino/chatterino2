// SPDX-FileCopyrightText: 2026 Contributors to Chatterino <https://chatterino.com>
//
// SPDX-License-Identifier: MIT

#include "singletons/Theme.hpp"

#include <QFile>
#include <QWidget>

#include <cassert>

namespace chatterino {

inline auto loadSettingsStylesheet()
{
    const auto *themeFile = getTheme()->isLightTheme()
        ? ":/qss/settingsLight.qss"
        : ":/qss/settingsDark.qss";
    QFile styleFile(themeFile);
    if (!styleFile.open(QFile::ReadOnly))
    {
        assert(false && "Resources not loaded");
        qCWarning(chatterinoWidget) << "Resources not loaded";
    }

    return QString::fromUtf8(styleFile.readAll());
}

}  // namespace chatterino
