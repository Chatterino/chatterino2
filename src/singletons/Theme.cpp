#include "singletons/Theme.hpp"

#include "Application.hpp"
#include "common/Literals.hpp"
#include "common/QLogging.hpp"
#include "singletons/Paths.hpp"
#include "singletons/Resources.hpp"
#include "singletons/WindowManager.hpp"

#include <QColor>
#include <QDir>
#include <QElapsedTimer>
#include <QFile>
#include <QJsonArray>
#include <QJsonDocument>
#include <QSet>
#if QT_VERSION >= QT_VERSION_CHECK(6, 5, 0)
#    include <QStyleHints>
#endif
#include <QApplication>

#include <cmath>

namespace {

using namespace chatterino;
using namespace literals;

void parseInto(const QJsonObject &obj, const QJsonObject &fallbackObj,
               QLatin1String key, QColor &color)
{
    auto parseColorFrom = [](const auto &obj,
                             QLatin1String key) -> std::optional<QColor> {
        auto jsonValue = obj[key];
        if (!jsonValue.isString()) [[unlikely]]
        {
            return std::nullopt;
        }
        QColor parsed = {jsonValue.toString()};
        if (!parsed.isValid()) [[unlikely]]
        {
            qCWarning(chatterinoTheme).nospace()
                << "While parsing " << key << ": '" << jsonValue.toString()
                << "' isn't a valid color.";
            return std::nullopt;
        }
        return parsed;
    };

    auto firstColor = parseColorFrom(obj, key);
    if (firstColor.has_value())
    {
        color = firstColor.value();
        return;
    }

    if (!fallbackObj.isEmpty())
    {
        auto fallbackColor = parseColorFrom(fallbackObj, key);
        if (fallbackColor.has_value())
        {
            color = fallbackColor.value();
            return;
        }
    }

    qCWarning(chatterinoTheme) << key
                               << "was expected but not found in the "
                                  "current theme, and no fallback value found.";
}

// NOLINTBEGIN(cppcoreguidelines-macro-usage)
#define _c2StringLit(s, ty) s##ty
#define parseColor(to, from, key) \
    parseInto(from, from##Fallback, _c2StringLit(#key, _L1), (to).from.key)
// NOLINTEND(cppcoreguidelines-macro-usage)

void parseWindow(const QJsonObject &window, const QJsonObject &windowFallback,
                 chatterino::Theme &theme)
{
    parseColor(theme, window, background);
    parseColor(theme, window, text);
}

void parseTabs(const QJsonObject &tabs, const QJsonObject &tabsFallback,
               chatterino::Theme &theme)
{
    const auto parseTabColors = [](const auto &json, const auto &jsonFallback,
                                   auto &tab) {
        parseInto(json, jsonFallback, "text"_L1, tab.text);
        {
            const auto backgrounds = json["backgrounds"_L1].toObject();
            const auto backgroundsFallback =
                jsonFallback["backgrounds"_L1].toObject();
            parseColor(tab, backgrounds, regular);
            parseColor(tab, backgrounds, hover);
            parseColor(tab, backgrounds, unfocused);
        }
        {
            const auto line = json["line"_L1].toObject();
            const auto lineFallback = jsonFallback["line"_L1].toObject();
            parseColor(tab, line, regular);
            parseColor(tab, line, hover);
            parseColor(tab, line, unfocused);
        }
    };
    parseColor(theme, tabs, dividerLine);
    parseColor(theme, tabs, liveIndicator);
    parseColor(theme, tabs, rerunIndicator);
    parseTabColors(tabs["regular"_L1].toObject(),
                   tabsFallback["regular"_L1].toObject(), theme.tabs.regular);
    parseTabColors(tabs["newMessage"_L1].toObject(),
                   tabsFallback["newMessage"_L1].toObject(),
                   theme.tabs.newMessage);
    parseTabColors(tabs["highlighted"_L1].toObject(),
                   tabsFallback["highlighted"_L1].toObject(),
                   theme.tabs.highlighted);
    parseTabColors(tabs["selected"_L1].toObject(),
                   tabsFallback["selected"_L1].toObject(), theme.tabs.selected);
}

void parseTextColors(const QJsonObject &textColors,
                     const QJsonObject &textColorsFallback, auto &messages)
{
    parseColor(messages, textColors, regular);
    parseColor(messages, textColors, caret);
    parseColor(messages, textColors, link);
    parseColor(messages, textColors, system);
    parseColor(messages, textColors, chatPlaceholder);
}

void parseMessageBackgrounds(const QJsonObject &backgrounds,
                             const QJsonObject &backgroundsFallback,
                             auto &messages)
{
    parseColor(messages, backgrounds, regular);
    parseColor(messages, backgrounds, alternate);
}

void parseMessages(const QJsonObject &messages,
                   const QJsonObject &messagesFallback,
                   chatterino::Theme &theme)
{
    parseTextColors(messages["textColors"_L1].toObject(),
                    messagesFallback["textColors"_L1].toObject(),
                    theme.messages);
    parseMessageBackgrounds(messages["backgrounds"_L1].toObject(),
                            messagesFallback["backgrounds"_L1].toObject(),
                            theme.messages);
    parseColor(theme, messages, disabled);
    parseColor(theme, messages, selection);
    parseColor(theme, messages, highlightAnimationStart);
    parseColor(theme, messages, highlightAnimationEnd);
}

void parseOverlayMessages(const QJsonObject &overlayMessages,
                          const QJsonObject &overlayMessagesFallback,
                          chatterino::Theme &theme)
{
    parseTextColors(overlayMessages["textColors"_L1].toObject(),
                    overlayMessagesFallback["textColors"_L1].toObject(),
                    theme.overlayMessages);
    parseMessageBackgrounds(
        overlayMessages["backgrounds"_L1].toObject(),
        overlayMessagesFallback["backgrounds"_L1].toObject(),
        theme.overlayMessages);
    parseColor(theme, overlayMessages, disabled);
    parseColor(theme, overlayMessages, selection);
    parseColor(theme, overlayMessages, background);
}

void parseScrollbars(const QJsonObject &scrollbars,
                     const QJsonObject &scrollbarsFallback,
                     chatterino::Theme &theme)
{
    parseColor(theme, scrollbars, background);
    parseColor(theme, scrollbars, thumb);
    parseColor(theme, scrollbars, thumbSelected);
}

void parseSplits(const QJsonObject &splits, const QJsonObject &splitsFallback,
                 chatterino::Theme &theme)
{
    parseColor(theme, splits, messageSeperator);
    parseColor(theme, splits, background);
    parseColor(theme, splits, dropPreview);
    parseColor(theme, splits, dropPreviewBorder);
    parseColor(theme, splits, dropTargetRect);
    parseColor(theme, splits, dropTargetRectBorder);
    parseColor(theme, splits, resizeHandle);
    parseColor(theme, splits, resizeHandleBackground);

    {
        const auto header = splits["header"_L1].toObject();
        const auto headerFallback = splitsFallback["header"_L1].toObject();
        parseColor(theme.splits, header, border);
        parseColor(theme.splits, header, focusedBorder);
        parseColor(theme.splits, header, background);
        parseColor(theme.splits, header, focusedBackground);
        parseColor(theme.splits, header, text);
        parseColor(theme.splits, header, focusedText);
    }
    {
        const auto input = splits["input"_L1].toObject();
        const auto inputFallback = splitsFallback["input"_L1].toObject();
        parseColor(theme.splits, input, background);
        parseColor(theme.splits, input, text);
    }
}

void parseColors(const QJsonObject &root, const QJsonObject &fallbackTheme,
                 chatterino::Theme &theme)
{
    const auto colors = root["colors"_L1].toObject();
    const auto fallbackColors = fallbackTheme["colors"_L1].toObject();

    parseInto(colors, fallbackColors, "accent"_L1, theme.accent);

    parseWindow(colors["window"_L1].toObject(),
                fallbackColors["window"_L1].toObject(), theme);
    parseTabs(colors["tabs"_L1].toObject(),
              fallbackColors["tabs"_L1].toObject(), theme);
    parseMessages(colors["messages"_L1].toObject(),
                  fallbackColors["messages"_L1].toObject(), theme);
    parseOverlayMessages(colors["overlayMessages"_L1].toObject(),
                         fallbackColors["overlayMessages"_L1].toObject(),
                         theme);
    parseScrollbars(colors["scrollbars"_L1].toObject(),
                    fallbackColors["scrollbars"_L1].toObject(), theme);
    parseSplits(colors["splits"_L1].toObject(),
                fallbackColors["splits"_L1].toObject(), theme);
}
#undef parseColor
#undef _c2StringLit

std::optional<QJsonObject> loadThemeFromPath(const QString &path)
{
    QFile file(path);
    if (!file.open(QFile::ReadOnly))
    {
        qCWarning(chatterinoTheme)
            << "Failed to open" << file.fileName() << "at" << path;
        return std::nullopt;
    }

    QJsonParseError error{};
    auto json = QJsonDocument::fromJson(file.readAll(), &error);
    if (!json.isObject())
    {
        qCWarning(chatterinoTheme) << "Failed to parse" << file.fileName()
                                   << "error:" << error.errorString();
        return std::nullopt;
    }

    // TODO: Validate JSON schema?

    return json.object();
}

/**
 * Load the given theme descriptor from its path
 *
 * Returns a JSON object containing theme data if the theme is valid, otherwise nullopt
 *
 * NOTE: No theme validation is done by this function
 **/
std::optional<QJsonObject> loadTheme(const ThemeDescriptor &theme)
{
    return loadThemeFromPath(theme.path);
}

}  // namespace

namespace chatterino {

const std::vector<ThemeDescriptor> Theme::builtInThemes{
    {
        .key = "White",
        .path = ":/themes/White.json",
        .name = "White",
    },
    {
        .key = "Light",
        .path = ":/themes/Light.json",
        .name = "Light",
    },
    {
        .key = "Dark",
        .path = ":/themes/Dark.json",
        .name = "Dark",
    },
    {
        .key = "Black",
        .path = ":/themes/Black.json",
        .name = "Black",
    },
};

// Dark is our default & fallback theme
const ThemeDescriptor Theme::fallbackTheme = Theme::builtInThemes.at(2);

bool Theme::isLightTheme() const
{
    return this->isLight_;
}

bool Theme::isSystemTheme() const
{
    return this->themeName == u"System"_s;
}

Theme::Theme(const Paths &paths)
{
    this->themeName.connect(
        [this](auto themeName) {
            qCInfo(chatterinoTheme) << "Theme updated to" << themeName;
            this->update();
        },
        false);
    auto updateIfSystem = [this](const auto &) {
        if (this->isSystemTheme())
        {
            this->update();
        }
    };
    this->darkSystemThemeName.connect(updateIfSystem, false);
    this->lightSystemThemeName.connect(updateIfSystem, false);

    this->loadAvailableThemes(paths);

#if QT_VERSION >= QT_VERSION_CHECK(6, 5, 0)
    QObject::connect(QApplication::styleHints(),
                     &QStyleHints::colorSchemeChanged, &this->lifetime_,
                     [this] {
                         if (this->isSystemTheme())
                         {
                             this->update();
                             getApp()->getWindows()->forceLayoutChannelViews();
                         }
                     });
#endif

    this->update();
}

void Theme::update()
{
    auto currentTheme = [&]() -> QString {
#if QT_VERSION >= QT_VERSION_CHECK(6, 5, 0)
        if (this->isSystemTheme())
        {
            switch (QApplication::styleHints()->colorScheme())
            {
                case Qt::ColorScheme::Light:
                    return this->lightSystemThemeName;
                case Qt::ColorScheme::Unknown:
                case Qt::ColorScheme::Dark:
                    return this->darkSystemThemeName;
            }
        }
#endif
        return this->themeName;
    };

    auto oTheme = this->findThemeByKey(currentTheme());

    constexpr const double nsToMs = 1.0 / 1000000.0;
    QElapsedTimer timer;
    timer.start();

    std::optional<QJsonObject> themeJSON;
    QString themePath;
    bool isCustomTheme = false;
    if (!oTheme)
    {
        qCWarning(chatterinoTheme)
            << "Theme" << this->themeName
            << "not found, falling back to the fallback theme";

        themeJSON = loadTheme(fallbackTheme);
        themePath = fallbackTheme.path;
    }
    else
    {
        const auto &theme = *oTheme;

        themeJSON = loadTheme(theme);
        themePath = theme.path;

        if (!themeJSON)
        {
            qCWarning(chatterinoTheme)
                << "Theme" << this->themeName
                << "not valid, falling back to the fallback theme";

            // Parsing the theme failed, fall back
            themeJSON = loadTheme(fallbackTheme);
            themePath = fallbackTheme.path;
        }
        else
        {
            isCustomTheme = theme.custom;
        }
    }
    auto loadTs = double(timer.nsecsElapsed()) * nsToMs;

    if (!themeJSON)
    {
        qCWarning(chatterinoTheme)
            << "Failed to load" << this->themeName << "or the fallback theme";
        return;
    }

    if (this->isAutoReloading() && this->currentThemeJson_ == *themeJSON)
    {
        return;
    }

    this->parseFrom(*themeJSON, isCustomTheme);
    this->currentThemePath_ = themePath;

    auto parseTs = double(timer.nsecsElapsed()) * nsToMs;

    this->updated.invoke();
    auto updateTs = double(timer.nsecsElapsed()) * nsToMs;
    qCDebug(chatterinoTheme).nospace().noquote()
        << "Updated theme in " << QString::number(updateTs, 'f', 2)
        << "ms (load: " << QString::number(loadTs, 'f', 2)
        << "ms, parse: " << QString::number(parseTs - loadTs, 'f', 2)
        << "ms, update: " << QString::number(updateTs - parseTs, 'f', 2)
        << "ms)";

    if (this->isAutoReloading())
    {
        this->currentThemeJson_ = *themeJSON;
    }
}

std::vector<std::pair<QString, QVariant>> Theme::availableThemes() const
{
    std::vector<std::pair<QString, QVariant>> packagedThemes;

    for (const auto &theme : this->availableThemes_)
    {
        if (theme.custom)
        {
            auto p = std::make_pair(
                QStringLiteral("Custom: %1").arg(theme.name), theme.key);

            packagedThemes.emplace_back(p);
        }
        else
        {
            auto p = std::make_pair(theme.name, theme.key);

            packagedThemes.emplace_back(p);
        }
    }

    return packagedThemes;
}

void Theme::loadAvailableThemes(const Paths &paths)
{
    this->availableThemes_ = Theme::builtInThemes;

    auto dir = QDir(paths.themesDirectory);
    for (const auto &info :
         dir.entryInfoList(QDir::Files | QDir::NoDotAndDotDot, QDir::Name))
    {
        if (!info.isFile())
        {
            continue;
        }

        if (!info.fileName().endsWith(".json"))
        {
            continue;
        }

        auto themeName = info.baseName();

        auto themeDescriptor = ThemeDescriptor{
            info.fileName(), info.absoluteFilePath(), themeName, true};

        auto theme = loadTheme(themeDescriptor);
        if (!theme)
        {
            qCWarning(chatterinoTheme) << "Failed to parse theme at" << info;
            continue;
        }

        this->availableThemes_.emplace_back(std::move(themeDescriptor));
    }
}

std::optional<ThemeDescriptor> Theme::findThemeByKey(const QString &key)
{
    for (const auto &theme : this->availableThemes_)
    {
        if (theme.key == key)
        {
            return theme;
        }
    }

    return std::nullopt;
}

void Theme::parseFrom(const QJsonObject &root, bool isCustomTheme)
{
    this->isLight_ =
        root["metadata"_L1]["iconTheme"_L1].toString() == u"dark"_s;

    std::optional<QJsonObject> fallbackTheme;
    if (isCustomTheme)
    {
        // Only attempt to load a fallback theme if the theme we're loading is a custom theme
        auto fallbackThemeName =
            root["metadata"_L1]["fallbackTheme"_L1].toString(
                this->isLightTheme() ? "Light" : "Dark");
        for (const auto &theme : Theme::builtInThemes)
        {
            if (fallbackThemeName.compare(theme.key, Qt::CaseInsensitive) == 0)
            {
                fallbackTheme = loadTheme(theme);
                break;
            }
        }
    }

    parseColors(root, fallbackTheme.value_or(QJsonObject()), *this);

    this->splits.input.styleSheet = uR"(
        background: %1;
        border: %2;
        color: %3;
        selection-background-color: %4;
    )"_s.arg(
        this->splits.input.background.name(QColor::HexArgb),
        this->tabs.selected.backgrounds.regular.name(QColor::HexArgb),
        this->messages.textColors.regular.name(QColor::HexArgb),
        this->isLightTheme()
            ? u"#68B1FF"_s
            : this->tabs.selected.backgrounds.regular.name(QColor::HexArgb));

    // Usercard buttons
    if (this->isLightTheme())
    {
        this->buttons.copy = getResources().buttons.copyDark;
        this->buttons.pin = getResources().buttons.pinDisabledDark;
    }
    else
    {
        this->buttons.copy = getResources().buttons.copyLight;
        this->buttons.pin = getResources().buttons.pinDisabledLight;
    }
}

bool Theme::isAutoReloading() const
{
    return this->themeReloadTimer_ != nullptr;
}

void Theme::setAutoReload(bool autoReload)
{
    if (autoReload == this->isAutoReloading())
    {
        return;
    }

    if (!autoReload)
    {
        this->themeReloadTimer_.reset();
        this->currentThemeJson_ = {};
        return;
    }

    this->themeReloadTimer_ = std::make_unique<QTimer>();
    QObject::connect(this->themeReloadTimer_.get(), &QTimer::timeout, [this]() {
        this->update();
    });
    this->themeReloadTimer_->setInterval(Theme::AUTO_RELOAD_INTERVAL_MS);
    this->themeReloadTimer_->start();

    qCDebug(chatterinoTheme) << "Enabled theme watcher";
}

void Theme::normalizeColor(QColor &color) const
{
    if (this->isLightTheme())
    {
        if (color.lightnessF() > 0.5)
        {
            color.setHslF(color.hueF(), color.saturationF(), 0.5);
        }

        if (color.lightnessF() > 0.4 && color.hueF() > 0.1 &&
            color.hueF() < 0.33333)
        {
            color.setHslF(color.hueF(), color.saturationF(),
                          color.lightnessF() - sin((color.hueF() - 0.1) /
                                                   (0.3333 - 0.1) * 3.14159) *
                                                   color.saturationF() * 0.4);
        }
    }
    else
    {
        if (color.lightnessF() < 0.5)
        {
            color.setHslF(color.hueF(), color.saturationF(), 0.5);
        }

        if (color.lightnessF() < 0.6 && color.hueF() > 0.54444 &&
            color.hueF() < 0.83333)
        {
            color.setHslF(
                color.hueF(), color.saturationF(),
                color.lightnessF() + sin((color.hueF() - 0.54444) /
                                         (0.8333 - 0.54444) * 3.14159) *
                                         color.saturationF() * 0.4);
        }
    }
}

Theme *getTheme()
{
    return getApp()->getThemes();
}

}  // namespace chatterino
