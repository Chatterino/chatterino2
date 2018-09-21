#define LOOKUP_COLOR_COUNT 360

#include "singletons/Theme.hpp"

#include <QColor>

#include <cmath>

namespace chatterino {

namespace detail {

    double getMultiplierByTheme(const QString &themeName)
    {
        if (themeName == "Light") {
            return 0.8;
        } else if (themeName == "White") {
            return 1.0;
        } else if (themeName == "Black") {
            return -1.0;
        } else if (themeName == "Dark") {
            return -0.8;
        }

        return -0.8;
    }

}  // namespace detail

Theme::Theme()
    : themeName("/appearance/theme/name", "Dark")
    , themeHue("/appearance/theme/hue", 0.0)
{
    this->update();

    this->themeName.connectSimple([this](auto) { this->update(); }, false);
    this->themeHue.connectSimple([this](auto) { this->update(); }, false);
}

void Theme::update()
{
    this->actuallyUpdate(this->themeHue, detail::getMultiplierByTheme(
                                             this->themeName.getValue()));
}

// hue: theme color (0 - 1)
// multiplier: 1 = white, 0.8 = light, -0.8 dark, -1 black
void Theme::actuallyUpdate(double hue, double multiplier)
{
    isLight_ = multiplier > 0;
    bool lightWin = isLight_;

    //    QColor themeColor = QColor::fromHslF(hue, 0.43, 0.5);
    QColor themeColor = QColor::fromHslF(hue, 0.8, 0.5);
    QColor themeColorNoSat = QColor::fromHslF(hue, 0, 0.5);

    qreal sat = 0;
    //    0.05;

    auto getColor = [multiplier](double h, double s, double l, double a = 1.0) {
        return QColor::fromHslF(h, s, ((l - 0.5) * multiplier) + 0.5, a);
    };

    /// WINDOW
    {
        QColor bg =
#ifdef Q_OS_LINUX
            this->window.background = lightWin ? "#fff" : QColor(61, 60, 56);
#else
            this->window.background = lightWin ? "#fff" : "#111";
#endif

        QColor fg = this->window.text = lightWin ? "#000" : "#eee";
        this->window.borderFocused = lightWin ? "#ccc" : themeColor;
        this->window.borderUnfocused = lightWin ? "#ccc" : themeColorNoSat;

        // Ubuntu style
        // TODO: add setting for this
        //        TabText = QColor(210, 210, 210);
        //        TabBackground = QColor(61, 60, 56);
        //        TabHoverText = QColor(210, 210, 210);
        //        TabHoverBackground = QColor(73, 72, 68);

        // message (referenced later)
        this->messages.textColors.caret =  //
            this->messages.textColors.regular = isLight_ ? "#000" : "#fff";

        QColor highlighted = lightWin ? QColor("#ff0000") : QColor("#ee6166");

        /// TABS
        if (lightWin) {
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
                {QColor("#00aeef"), QColor("#00aeef"), QColor("#00aeef")}};
            this->tabs.notified = {
                fg,
                {QColor("#fff"), QColor("#fff"), QColor("#fff")},
                {QColor("#F824A8"), QColor("#F824A8"), QColor("#F824A8")}};
        } else {
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
                {QColor("#00aeef"), QColor("#00aeef"), QColor("#00aeef")}};
            this->tabs.notified = {
                fg,
                {QColor("#252525"), QColor("#252525"), QColor("#252525")},
                {QColor("#F824A8"), QColor("#F824A8"), QColor("#F824A8")}};
        }

        this->splits.input.focusedLine = highlighted;

        // scrollbar
        this->scrollbars.highlights.highlight = QColor("#ee6166");
        this->scrollbars.highlights.subscription = QColor("#C466FF");

        // this->tabs.newMessage = {
        //     fg,
        //     {QBrush(blendColors(themeColor, "#ccc", 0.9), Qt::FDiagPattern),
        //      QBrush(blendColors(themeColor, "#ccc", 0.9), Qt::FDiagPattern),
        //      QBrush(blendColors(themeColorNoSat, "#ccc", 0.9),
        //      Qt::FDiagPattern)}};

        //         this->tabs.newMessage = {
        //                fg,
        //                {QBrush(blendColors(themeColor, "#666", 0.7),
        //                Qt::FDiagPattern),
        //                 QBrush(blendColors(themeColor, "#666", 0.5),
        //                 Qt::FDiagPattern),
        //                 QBrush(blendColors(themeColorNoSat, "#666", 0.7),
        //                 Qt::FDiagPattern)}};
        //            this->tabs.highlighted = {fg, {QColor("#777"),
        //            QColor("#777"), QColor("#666")}};

        this->tabs.bottomLine = this->tabs.selected.backgrounds.regular.color();
    }  // namespace chatterino

    // Split
    bool flat = isLight_;

    this->splits.messageSeperator =
        isLight_ ? QColor(127, 127, 127) : QColor(60, 60, 60);
    this->splits.background = getColor(0, sat, 1);
    this->splits.dropPreview = QColor(0, 148, 255, 0x30);
    this->splits.dropPreviewBorder = QColor(0, 148, 255, 0xff);

    if (isLight_) {
        this->splits.dropTargetRect = QColor(255, 255, 255, 0x00);
        this->splits.dropTargetRectBorder = QColor(0, 148, 255, 0x00);

        this->splits.resizeHandle = QColor(0, 148, 255, 0xff);
        this->splits.resizeHandleBackground = QColor(0, 148, 255, 0x50);
    } else {
        this->splits.dropTargetRect = QColor(0, 148, 255, 0x00);
        this->splits.dropTargetRectBorder = QColor(0, 148, 255, 0x00);

        this->splits.resizeHandle = QColor(0, 148, 255, 0x70);
        this->splits.resizeHandleBackground = QColor(0, 148, 255, 0x20);
    }

    this->splits.header.background = getColor(0, sat, flat ? 1 : 0.9);
    this->splits.header.border = getColor(0, sat, flat ? 1 : 0.85);
    this->splits.header.text = this->messages.textColors.regular;
    this->splits.header.focusedText =
        isLight_ ? QColor("#198CFF") : QColor("#84C1FF");

    this->splits.input.background = getColor(0, sat, flat ? 0.95 : 0.95);
    this->splits.input.border = getColor(0, sat, flat ? 1 : 1);
    this->splits.input.text = this->messages.textColors.regular;
    this->splits.input.styleSheet =
        "background:" + this->splits.input.background.name() + ";" +
        "border:" + this->tabs.selected.backgrounds.regular.color().name() +
        ";" + "color:" + this->messages.textColors.regular.name() + ";" +  //
        "selection-background-color:" +
        (isLight_ ? "#68B1FF"
                  : this->tabs.selected.backgrounds.regular.color().name());

    // Message
    this->messages.textColors.link =
        isLight_ ? QColor(66, 134, 244) : QColor(66, 134, 244);
    this->messages.textColors.system = QColor(140, 127, 127);

    this->messages.backgrounds.regular = splits.background;
    this->messages.backgrounds.alternate = getColor(0, sat, 0.93);

    if (isLight_) {
        this->messages.backgrounds.highlighted =
            blendColors(themeColor, this->messages.backgrounds.regular, 0.8);
    } else {
        this->messages.backgrounds.highlighted = QColor(75, 40, 44);
    }

    this->messages.backgrounds.subscription =
        blendColors(QColor("#C466FF"), this->messages.backgrounds.regular, 0.7);

    // this->messages.backgrounds.resub
    // this->messages.backgrounds.whisper
    this->messages.disabled = getColor(0, sat, 1, 0.6);
    // this->messages.seperator =
    // this->messages.seperatorInner =

    // Scrollbar
    this->scrollbars.background = QColor(0, 0, 0, 0);
    //    this->scrollbars.background = splits.background;
    //    this->scrollbars.background.setAlphaF(qreal(0.2));
    this->scrollbars.thumb = getColor(0, sat, 0.70);
    this->scrollbars.thumbSelected = getColor(0, sat, 0.65);

    // tooltip
    this->tooltip.background = QColor(0, 0, 0);
    this->tooltip.text = QColor(255, 255, 255);

    // Selection
    this->messages.selection =
        isLightTheme() ? QColor(0, 0, 0, 64) : QColor(255, 255, 255, 64);

    this->updated.invoke();
}  // namespace chatterino

QColor Theme::blendColors(const QColor &color1, const QColor &color2,
                          qreal ratio)
{
    int r = int(color1.red() * (1 - ratio) + color2.red() * ratio);
    int g = int(color1.green() * (1 - ratio) + color2.green() * ratio);
    int b = int(color1.blue() * (1 - ratio) + color2.blue() * ratio);

    return QColor(r, g, b, 255);
}

void Theme::normalizeColor(QColor &color)
{
    if (this->isLight_) {
        if (color.lightnessF() > 0.5) {
            color.setHslF(color.hueF(), color.saturationF(), 0.5);
        }

        if (color.lightnessF() > 0.4 && color.hueF() > 0.1 &&
            color.hueF() < 0.33333) {
            color.setHslF(color.hueF(), color.saturationF(),
                          color.lightnessF() - sin((color.hueF() - 0.1) /
                                                   (0.3333 - 0.1) * 3.14159) *
                                                   color.saturationF() * 0.4);
        }
    } else {
        if (color.lightnessF() < 0.5) {
            color.setHslF(color.hueF(), color.saturationF(), 0.5);
        }

        if (color.lightnessF() < 0.6 && color.hueF() > 0.54444 &&
            color.hueF() < 0.83333) {
            color.setHslF(
                color.hueF(), color.saturationF(),
                color.lightnessF() + sin((color.hueF() - 0.54444) /
                                         (0.8333 - 0.54444) * 3.14159) *
                                         color.saturationF() * 0.4);
        }
    }
}

}  // namespace chatterino
