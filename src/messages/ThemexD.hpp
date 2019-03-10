#pragma once

#include <QMap>
#include <QObject>
#include <QStyle>
#include <QWidget>

namespace messages
{
    class Text : public QWidget
    {
        Q_OBJECT

    public:
        Text(QWidget* parent)
            : QWidget(parent)
        {
        }
    };

    class Message : public QWidget
    {
        Q_OBJECT

    public:
        Message(QWidget* parent)
            : QWidget(parent)
        {
        }
    };
}  // namespace messages

namespace chatterino
{
    class ThemexD
    {
    public:
        ThemexD(QWidget* host);

        void updateStyle();

        QBrush getMessageBackground(const QStringList& properties);
        QBrush getTextColor(const QStringList& properties);
        QFont getFont(const QStringList& properties);

    private:
        QMap<QStringList, QBrush> messageBackgrounds;
        QMap<QStringList, QBrush> textColors;
        QMap<QStringList, QFont> fonts;

        QWidget* host;
        messages::Message message;
        messages::Text text;
    };
}  // namespace chatterino
