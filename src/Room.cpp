#include "Room.hpp"

#include <QTimer>
#include <memory>

#include <QLabel>

namespace chatterino
{
    Room::Room()
    {
        this->setTitle("empty");
    }

    QString Room::title() const
    {
        return this->title_;
    }

    void Room::setTitle(const QString& str)
    {
        this->title_ = str;

        emit this->titleChanged(str);
    }

    int Room::calcInputLength(const QString& string) const
    {
        return string.length();
    }

    QWidget* Room::createView(QWidget* parent) const
    {
        return new QLabel("empty channel", parent);
    }

    void Room::fillDropdown(ui::Dropdown&) const
    {
    }

    QJsonObject Room::serialize() const
    {
        return {};
    }

    RoomPtr emptyRoom()
    {
        static RoomPtr empty = RoomPtr(new Room());

        return empty;
    }
}  // namespace chatterino
