#pragma once

#include <QFont>
#include <QFontMetrics>
#include <pajlada/settings/setting.hpp>
#include <pajlada/signals/signal.hpp>

namespace chatterino {

class FontManager
{
    FontManager(const FontManager &) = delete;
    FontManager(FontManager &&) = delete;
    FontManager();

public:
    enum Type : uint8_t {
        Small,
        MediumSmall,
        Medium,
        MediumBold,
        MediumItalic,
        Large,
        VeryLarge,
    };

    // FontManager is initialized only once, on first use
    static FontManager &getInstance();

    QFont &getFont(Type type, float dpi);
    QFontMetrics &getFontMetrics(Type type, float dpi);

    int getGeneration() const
    {
        return this->generation;
    }

    void incGeneration()
    {
        this->generation++;
    }

    pajlada::Settings::Setting<std::string> currentFontFamily;
    pajlada::Settings::Setting<int> currentFontSize;

    pajlada::Signals::NoArgSignal fontChanged;

private:
    struct FontData {
        FontData(QFont &&_font)
            : font(_font)
            , metrics(this->font)
        {
        }

        QFont font;
        QFontMetrics metrics;
    };

    struct Font {
        Font() = delete;

        explicit Font(const char *fontFamilyName, int mediumSize)
            : small(QFont(fontFamilyName, mediumSize - 4))
            , mediumSmall(QFont(fontFamilyName, mediumSize - 2))
            , medium(QFont(fontFamilyName, mediumSize))
            , mediumBold(QFont(fontFamilyName, mediumSize, QFont::DemiBold))
            , mediumItalic(QFont(fontFamilyName, mediumSize, -1, true))
            , large(QFont(fontFamilyName, mediumSize))
            , veryLarge(QFont(fontFamilyName, mediumSize))
        {
        }

        void setFamily(const char *newFamily)
        {
            this->small.font.setFamily(newFamily);
            this->mediumSmall.font.setFamily(newFamily);
            this->medium.font.setFamily(newFamily);
            this->mediumBold.font.setFamily(newFamily);
            this->mediumItalic.font.setFamily(newFamily);
            this->large.font.setFamily(newFamily);
            this->veryLarge.font.setFamily(newFamily);

            this->updateMetrics();
        }

        void setSize(int newMediumSize)
        {
            this->small.font.setPointSize(newMediumSize - 4);
            this->mediumSmall.font.setPointSize(newMediumSize - 2);
            this->medium.font.setPointSize(newMediumSize);
            this->mediumBold.font.setPointSize(newMediumSize);
            this->mediumItalic.font.setPointSize(newMediumSize);
            this->large.font.setPointSize(newMediumSize + 2);
            this->veryLarge.font.setPointSize(newMediumSize + 4);

            this->updateMetrics();
        }

        void updateMetrics()
        {
            this->small.metrics = QFontMetrics(this->small.font);
            this->mediumSmall.metrics = QFontMetrics(this->mediumSmall.font);
            this->medium.metrics = QFontMetrics(this->medium.font);
            this->mediumBold.metrics = QFontMetrics(this->mediumBold.font);
            this->mediumItalic.metrics = QFontMetrics(this->mediumItalic.font);
            this->large.metrics = QFontMetrics(this->large.font);
            this->veryLarge.metrics = QFontMetrics(this->veryLarge.font);
        }

        FontData &getFontData(Type type);

        QFont &getFont(Type type);
        QFontMetrics &getFontMetrics(Type type);

        FontData small;
        FontData mediumSmall;
        FontData medium;
        FontData mediumBold;
        FontData mediumItalic;
        FontData large;
        FontData veryLarge;
    };

    Font &getCurrentFont(float dpi);

    // Future plans:
    // Could have multiple fonts in here, such as "Menu font", "Application font", "Chat font"

    std::list<std::pair<float, Font>> currentFontByDpi;

    int generation = 0;
};

}  // namespace chatterino
