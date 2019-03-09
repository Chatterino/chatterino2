#include "Room.hpp"

namespace chatterino
{
    class TwitchRoomData;
    class MessageContainer;

    struct TwitchRoomModes
    {
        bool emoteOnly{};
        bool subOnly{};
        int slowMode{};
        bool r9k{};
        QString broadcasterLanguage{};
    };

    class TwitchRoom : public Room
    {
        Q_OBJECT

    public:
        TwitchRoom(const QString& name);
        ~TwitchRoom() override;

        void setUserId(const QString&);
        QString userId() const;

        void setModes(const TwitchRoomModes&);
        const TwitchRoomModes& modes() const;

        void setMod(bool);
        bool isMod() const;

        void addChatter(const QString&);
        void removeChatter(const QString&);

        MessageContainer& messages();

        // override
        QJsonObject serialize() const override;
        QWidget* createView(QWidget* parent) const override;
        void fillDropdown(ui::Dropdown&) const override;

    signals:
        void userIdChanged(const QString& value);
        void modesChanged(const TwitchRoomModes& value);

    private:
        TwitchRoomData* this_;
    };

    class TwitchMentionsRoom : public Room
    {
    public:
        TwitchMentionsRoom();

        QJsonObject serialize() const override;
        QWidget* createView(QWidget* parent) const override;
        void fillDropdown(ui::Dropdown&) const override;

        MessageContainer& messages();

    private:
        std::shared_ptr<MessageContainer> messages_;
    };

    class TwitchWhispersRoom : public Room
    {
    public:
        TwitchWhispersRoom();

        QJsonObject serialize() const override;
        QWidget* createView(QWidget* parent) const override;
        void fillDropdown(ui::Dropdown&) const override;

        MessageContainer& messages();

    private:
        std::shared_ptr<MessageContainer> messages_;
    };
}  // namespace chatterino
