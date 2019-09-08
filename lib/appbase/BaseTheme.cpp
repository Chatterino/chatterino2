#include "BaseTheme.hpp"

namespace AB_NAMESPACE {
namespace {
    double getMultiplierByTheme(const QString &themeName)
    {
        if (themeName == "Light")
        {
            return 0.8;
        }
        else if (themeName == "White")
        {
            return 1.0;
        }
        else if (themeName == "Black")
        {
            return -1.0;
        }
        else if (themeName == "Dark")
        {
            return -0.8;
        }
        /*
        else if (themeName == "Custom")
        {
            return getSettings()->customThemeMultiplier.getValue();
        }
        */

        return -0.8;
    }
}  // namespace

bool AB_THEME_CLASS::isLightTheme() const
{
    return this->isLight_;
}

void AB_THEME_CLASS::update()
{
    this->actuallyUpdate(this->themeHue,
                         getMultiplierByTheme(this->themeName.getValue()));

    this->updated.invoke();
}

void AB_THEME_CLASS::actuallyUpdate(double hue, double multiplier)
{
    this->isLight_ = multiplier > 0;
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
                {QColor("#00aeef"), QColor("#00aeef"), QColor("#00aeef")}};
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
                {QColor("#00aeef"), QColor("#00aeef"), QColor("#00aeef")}};
        }

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
    }  // namespace AB_NAMESPACE

    // Split
    bool flat = isLight_;

    // Message
    this->messages.textColors.link =
        isLight_ ? QColor(66, 134, 244) : QColor(66, 134, 244);
    this->messages.textColors.system = QColor(140, 127, 127);

    this->messages.backgrounds.regular = getColor(0, sat, 1);
    this->messages.backgrounds.alternate = getColor(0, sat, 0.96);

    if (isLight_)
    {
        this->messages.backgrounds.highlighted =
            blendColors(themeColor, this->messages.backgrounds.regular, 0.8);
    }
    else
    {
        // REMOVED
        // this->messages.backgrounds.highlighted =
        //    QColor(getSettings()->highlightColor);
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
}

QColor AB_THEME_CLASS::blendColors(const QColor &color1, const QColor &color2,
                                   qreal ratio)
{
    int r = int(color1.red() * (1 - ratio) + color2.red() * ratio);
    int g = int(color1.green() * (1 - ratio) + color2.green() * ratio);
    int b = int(color1.blue() * (1 - ratio) + color2.blue() * ratio);

    return QColor(r, g, b, 255);
}

#ifndef AB_CUSTOM_THEME
Theme *getTheme()
{
    static auto theme = [] {
        auto theme = new Theme();
        theme->update();
        return theme;
    }();

    return theme;
}
#endif

}  // namespace AB_NAMESPACE
