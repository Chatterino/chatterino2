
#include "singletons/Theme.hpp"

#include "Application.hpp"
#include "singletons/Resources.hpp"

#include <QColor>

#include <cmath>

namespace {
double getMultiplierByTheme(const QString &themeName)
{
    if (themeName == "Light")
    {
        return 0.8;
    }
    if (themeName == "White")
    {
        return 1.0;
    }
    if (themeName == "Black")
    {
        return -1.0;
    }
    if (themeName == "Dark")
    {
        return -0.8;
    }

    return -0.8;  // default: Dark
}
}  // namespace

namespace chatterino {

bool Theme::isLightTheme() const
{
    return this->isLight_;
}

Theme::Theme()
{
    this->update();

    this->themeName.connectSimple(
        [this](auto) {
            this->update();
        },
        false);
}

void Theme::update()
{
    this->actuallyUpdate(getMultiplierByTheme(this->themeName.getValue()));

    this->updated.invoke();
}

// multiplier: 1 = white, 0.8 = light, -0.8 dark, -1 black
void Theme::actuallyUpdate(double multiplier)
{
    this->isLight_ = multiplier > 0;

    const auto isLight = this->isLightTheme();

    auto getGray = [multiplier](double l, double a = 1.0) {
        return QColor::fromHslF(0, 0, ((l - 0.5) * multiplier) + 0.5, a);
    };

    /// WINDOW
#ifdef Q_OS_LINUX
    this->window.background = isLight ? "#fff" : QColor(61, 60, 56);
#else
    this->window.background = isLight ? "#fff" : "#111";
#endif
    this->window.text = isLight ? "#000" : "#eee";

    // message (referenced later)
    this->messages.textColors.caret =  //
        this->messages.textColors.regular = isLight ? "#000" : "#fff";

    /// TABS

    // creates a `TabColors::line`, where all colors are the same
    auto tabLine = [](auto color) -> decltype(TabColors::line) {
        return {.regular = color, .hover = color, .unfocused = color};
    };
    // creates a `TabColors::backgrounds`, where all colors are the same
    auto tabBackground = [](auto color) -> decltype(TabColors::backgrounds) {
        return {.regular = color, .hover = color, .unfocused = color};
    };

    if (isLight)
    {
        this->tabs.regular = {
            .text = QColor("#444"),
            .backgrounds = {Qt::white, "#eee", Qt::white},
            .line = tabLine(Qt::white),
        };
        this->tabs.newMessage = {
            .text = QColor("#222"),
            .backgrounds = {Qt::white, "#eee", Qt::white},
            .line = tabLine("#bbb"),
        };
        this->tabs.highlighted = {
            .text = Qt::black,
            .backgrounds = {Qt::white, "#eee", Qt::white},
            .line = tabLine("#ff0000"),
        };
        this->tabs.selected = {
            .text = Qt::black,
            .backgrounds = tabBackground("#b4d7ff"),
            .line = tabLine(this->accent),
        };
    }
    else
    {
        this->tabs.regular = {
            .text = QColor("#aaa"),
            .backgrounds = tabBackground("#252525"),
            .line = tabLine("#444"),
        };
        this->tabs.newMessage = {
            .text = QColor("#eee"),
            .backgrounds = tabBackground("#252525"),
            .line = tabLine("#888"),
        };
        this->tabs.highlighted = {
            .text = QColor("#eee"),
            .backgrounds = tabBackground("#252525"),
            .line = tabLine("#ee6166"),
        };
        this->tabs.selected = {
            .text = Qt::white,
            .backgrounds = tabBackground("#555555"),
            .line = tabLine(this->accent),
        };
    }

    this->tabs.dividerLine = this->tabs.selected.backgrounds.regular;

    // Message
    this->messages.textColors.link = QColor(66, 134, 244);
    this->messages.textColors.system = QColor(140, 127, 127);
    this->messages.textColors.chatPlaceholder =
        isLight ? QColor(175, 159, 159) : QColor(93, 85, 85);

    this->messages.backgrounds.regular = getGray(1);
    this->messages.backgrounds.alternate = getGray(0.96);

    this->messages.disabled = getGray(1, 0.6);

    int complementaryGray = this->isLightTheme() ? 20 : 230;
    this->messages.highlightAnimationStart =
        QColor(complementaryGray, complementaryGray, complementaryGray, 110);
    this->messages.highlightAnimationEnd =
        QColor(complementaryGray, complementaryGray, complementaryGray, 0);

    // Scrollbar
    this->scrollbars.background = QColor(0, 0, 0, 0);
    this->scrollbars.thumb = getGray(0.70);
    this->scrollbars.thumbSelected = getGray(0.65);

    // Selection
    this->messages.selection =
        isLightTheme() ? QColor(0, 0, 0, 64) : QColor(255, 255, 255, 64);

    if (this->isLightTheme())
    {
        this->splits.dropTargetRect = QColor(255, 255, 255, 0x00);
        this->splits.dropTargetRectBorder = QColor(0, 148, 255, 0x00);

        this->splits.resizeHandle = QColor(0, 148, 255, 0xff);
        this->splits.resizeHandleBackground = QColor(0, 148, 255, 0x50);
    }
    else
    {
        this->splits.dropTargetRect = QColor(0, 148, 255, 0x00);
        this->splits.dropTargetRectBorder = QColor(0, 148, 255, 0x00);

        this->splits.resizeHandle = QColor(0, 148, 255, 0x70);
        this->splits.resizeHandleBackground = QColor(0, 148, 255, 0x20);
    }

    this->splits.header.background = getGray(isLight ? 1 : 0.9);
    this->splits.header.border = getGray(isLight ? 1 : 0.85);
    this->splits.header.text = this->messages.textColors.regular;
    this->splits.header.focusedBackground = getGray(isLight ? 0.95 : 0.79);
    this->splits.header.focusedBorder = getGray(isLight ? 0.90 : 0.78);
    this->splits.header.focusedText = QColor::fromHsvF(
        0.58388, isLight ? 1.0 : 0.482, isLight ? 0.6375 : 1.0);

    this->splits.input.background = getGray(0.95);
    this->splits.input.text = this->messages.textColors.regular;
    this->splits.input.styleSheet =
        "background:" + this->splits.input.background.name() + ";" +
        "border:" + this->tabs.selected.backgrounds.regular.name() + ";" +
        "color:" + this->messages.textColors.regular.name() + ";" +
        "selection-background-color:" +
        (isLight ? "#68B1FF" : this->tabs.selected.backgrounds.regular.name());

    this->splits.messageSeperator =
        isLight ? QColor(127, 127, 127) : QColor(60, 60, 60);
    this->splits.background = getGray(1);
    this->splits.dropPreview = QColor(0, 148, 255, 0x30);
    this->splits.dropPreviewBorder = QColor(0, 148, 255, 0xff);

    // Copy button
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
    return getApp()->themes;
}

}  // namespace chatterino
