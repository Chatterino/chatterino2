#pragma once

#include <QObject>
#include <QString>

#include <QJsonObject>
#include <memory>

namespace chatterino::ui
{
    class Dropdown;
}

namespace chatterino
{
    class Room : public QObject
    {
        Q_OBJECT

    public:
        Room();
        virtual ~Room() = default;

        QString title() const;
        virtual int calcInputLength(const QString& text) const;
        virtual QWidget* createView(QWidget* parent) const;
        virtual void fillDropdown(ui::Dropdown&) const;

        virtual QJsonObject serialize() const;

        // TODO: consider if this is needed
        void repaint()
        {
        }

    signals:
        void titleChanged(const QString& title);

    protected:
        void setTitle(const QString&);

    private:
        QString title_;
    };

    using RoomPtr = std::shared_ptr<Room>;

    RoomPtr emptyRoom();
}  // namespace chatterino
