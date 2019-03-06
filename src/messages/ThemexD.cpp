#include "ThemexD.hpp"

#include <QFile>
#include <QStyle>
#include <QVariant>

namespace chatterino
{
    ThemexD::ThemexD(QWidget* host)
        : host(host)
        , message(host)
        , text(host)
    {
    }

    void ThemexD::updateStyle()
    {
        this->messageBackgrounds.clear();
        this->textColors.clear();
        this->fonts.clear();
    }

    QBrush ThemexD::getMessageBackground(const QStringList& properties)
    {
        // cache
        if (this->messageBackgrounds.contains(properties))
            return this->messageBackgrounds[properties];

        // set properties
        for (auto&& property : properties)
            this->message.setProperty(property.toStdString().data(), true);

        // get property
        this->host->style()->polish(&this->message);
        auto bg = this->message.palette().background();
        this->host->style()->unpolish(&this->message);

        // add to cache
        this->messageBackgrounds[properties] = bg;

        // unset properties
        for (auto&& property : properties)
            this->message.setProperty(
                property.toStdString().data(), QVariant());

        // return
        return bg;
    }

    QBrush ThemexD::getTextColor(const QStringList& properties)
    {
        // cache
        if (this->textColors.contains(properties))
            return this->textColors[properties];

        // set properties
        for (auto&& property : properties)
            this->text.setProperty(property.toStdString().data(), true);

        // get property
        this->host->style()->polish(&this->text);
        auto bg = this->text.palette().text();
        this->host->style()->unpolish(&this->text);

        // add to cache
        this->textColors[properties] = bg;

        // unset properties
        for (auto&& property : properties)
            this->text.setProperty(property.toStdString().data(), QVariant());

        // return
        return bg;
    }

    QFont ThemexD::getFont(const QStringList& properties)
    {
        // cache
        if (this->fonts.contains(properties))
            return this->fonts[properties];

        // set properties
        for (auto&& property : properties)
            this->text.setProperty(property.toStdString().data(), true);

        // get property
        this->host->style()->polish(&this->text);
        auto font = this->text.font();
        qDebug() << font;
        this->host->style()->unpolish(&this->text);

        // add to cache
        this->fonts[properties] = font;

        // unset properties
        for (auto&& property : properties)
            this->text.setProperty(property.toStdString().data(), QVariant());

        // return
        return font;
    }
}  // namespace chatterino
