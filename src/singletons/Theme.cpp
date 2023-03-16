
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
    bool lightWin = isLight_;

    const auto isLight = this->isLightTheme();

    auto getColor = [multiplier](double h, double s, double l, double a = 1.0) {
        return QColor::fromHslF(h, s, ((l - 0.5) * multiplier) + 0.5, a);
    };

    /// WINDOW
    {
#ifdef Q_OS_LINUX
        this->window.background = lightWin ? "#fff" : QColor(61, 60, 56);
#else
        this->window.background = lightWin ? "#fff" : "#111";
#endif

        QColor fg = this->window.text = lightWin ? "#000" : "#eee";

        // message (referenced later)
        this->messages.textColors.caret =  //
            this->messages.textColors.regular = isLight_ ? "#000" : "#fff";

        QColor highlighted = lightWin ? QColor("#ff0000") : QColor("#ee6166");

        /// TABS
        if (lightWin)
        {
            this->tabs.regular = {
                QColor("#444"),
                {QColor("#fff"), QColor("#eee"), QColor("#fff")},
                {QColor("#fff"), QColor("#fff"), QColor("#fff")}};
            this->tabs.newMessage = {
                QColor("#222"),
                {QColor("#fff"), QColor("#eee"), QColor("#fff")},
                {QColor("#bbb"), QColor("#bbb"), QColor("#bbb")}};
            this->tabs.highlighted = {
                fg,
                {QColor("#fff"), QColor("#eee"), QColor("#fff")},
                {highlighted, highlighted, highlighted}};
            this->tabs.selected = {
                QColor("#000"),
                {QColor("#b4d7ff"), QColor("#b4d7ff"), QColor("#b4d7ff")},
                {this->accent, this->accent, this->accent}};
        }
        else
        {
            this->tabs.regular = {
                QColor("#aaa"),
                {QColor("#252525"), QColor("#252525"), QColor("#252525")},
                {QColor("#444"), QColor("#444"), QColor("#444")}};
            this->tabs.newMessage = {
                fg,
                {QColor("#252525"), QColor("#252525"), QColor("#252525")},
                {QColor("#888"), QColor("#888"), QColor("#888")}};
            this->tabs.highlighted = {
                fg,
                {QColor("#252525"), QColor("#252525"), QColor("#252525")},
                {highlighted, highlighted, highlighted}};

            this->tabs.selected = {
                QColor("#fff"),
                {QColor("#555555"), QColor("#555555"), QColor("#555555")},
                {this->accent, this->accent, this->accent}};
        }

        this->tabs.dividerLine =
            this->tabs.selected.backgrounds.regular.color();
    }

    // Message
    this->messages.textColors.link =
        isLight_ ? QColor(66, 134, 244) : QColor(66, 134, 244);
    this->messages.textColors.system = QColor(140, 127, 127);
    this->messages.textColors.chatPlaceholder =
        isLight_ ? QColor(175, 159, 159) : QColor(93, 85, 85);

    this->messages.backgrounds.regular = getColor(0, 0, 1);
    this->messages.backgrounds.alternate = getColor(0, 0, 0.96);

    this->messages.disabled = getColor(0, 0, 1, 0.6);

    int complementaryGray = this->isLightTheme() ? 20 : 230;
    this->messages.highlightAnimationStart =
        QColor(complementaryGray, complementaryGray, complementaryGray, 110);
    this->messages.highlightAnimationEnd =
        QColor(complementaryGray, complementaryGray, complementaryGray, 0);

    // Scrollbar
    this->scrollbars.background = QColor(0, 0, 0, 0);
    this->scrollbars.thumb = getColor(0, 0, 0.70);
    this->scrollbars.thumbSelected = getColor(0, 0, 0.65);

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

    this->splits.header.background = getColor(0, 0, isLight ? 1 : 0.9);
    this->splits.header.border = getColor(0, 0, isLight ? 1 : 0.85);
    this->splits.header.text = this->messages.textColors.regular;
    this->splits.header.focusedBackground =
        getColor(0, 0, isLight ? 0.95 : 0.79);
    this->splits.header.focusedBorder = getColor(0, 0, isLight ? 0.90 : 0.78);
    this->splits.header.focusedText = QColor::fromHsvF(
        0.58388, isLight ? 1.0 : 0.482, isLight ? 0.6375 : 1.0);

    this->splits.input.background = getColor(0, 0, isLight ? 0.95 : 0.95);
    this->splits.input.text = this->messages.textColors.regular;
    this->splits.input.styleSheet =
        "background:" + this->splits.input.background.name() + ";" +
        "border:" + this->tabs.selected.backgrounds.regular.color().name() +
        ";" + "color:" + this->messages.textColors.regular.name() + ";" +
        "selection-background-color:" +
        (isLight ? "#68B1FF"
                 : this->tabs.selected.backgrounds.regular.color().name());

    this->splits.messageSeperator =
        isLight ? QColor(127, 127, 127) : QColor(60, 60, 60);
    this->splits.background = getColor(0, 0, 1);
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
