#pragma once

#include "ABTheme.hpp"
#include "common/Singleton.hpp"
#include "util/RapidJsonSerializeQString.hpp"

#include <QBrush>
#include <QColor>
#include <pajlada/settings/setting.hpp>
#include <singletons/Settings.hpp>

namespace chatterino {

class WindowManager;

class Theme final : public Singleton, public ABTheme
{
public:
    Theme();

    /// SPLITS
    struct {
        QColor messageSeperator;
        QColor background;
        QColor dropPreview;
        QColor dropPreviewBorder;
        QColor dropTargetRect;
        QColor dropTargetRectBorder;
        QColor resizeHandle;
        QColor resizeHandleBackground;

        struct {
            QColor border;
            QColor background;
            QColor text;
            QColor focusedText;
            // int margin;
        } header;

        struct {
            QColor border;
            QColor background;
            QColor selection;
            QColor focusedLine;
            QColor text;
            QString styleSheet;
            // int margin;
        } input;
    } splits;

    void normalizeColor(QColor &color);

private:
    void actuallyUpdate(double hue, double multiplier) override;
    void fillLookupTableValues(double (&array)[360], double from, double to,
                               double fromValue, double toValue);

    double middleLookupTable_[360] = {};
    double minLookupTable_[360] = {};

    pajlada::Signals::NoArgSignal repaintVisibleChatWidgets_;

    friend class WindowManager;
};

}  // namespace chatterino
