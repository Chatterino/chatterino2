#include "twitch/TwitchRoom.hpp"

#include "TwitchChannel.hpp"
#include "ui/ChannelView.hpp"
#include "ui/Dropdown.hpp"

#include <QDesktopServices>

namespace chatterino
{
    class TwitchRoomData
    {
    public:
        QString name;
        std::shared_ptr<TwitchChannel> channel;
        QString userId;
        TwitchRoomModes modes;
        bool isMod;
        std::shared_ptr<MessageContainer> messages;
    };

    TwitchRoom::TwitchRoom(const QString& name)
        : this_(new TwitchRoomData)
    {
        this_->name = name;
        this->setTitle(name);

        this_->messages = std::make_shared<MessageContainer>();
    }

    TwitchRoom::~TwitchRoom()
    {
        delete this_;
    }

    void TwitchRoom::setUserId(QString const& value)
    {
        if (this_->userId != value)
        {
            this_->userId = value;

            emit userIdChanged(value);
        }
    }

    QString TwitchRoom::userId() const
    {
        return this_->userId;
    }

    void TwitchRoom::setModes(TwitchRoomModes const& value)
    {
        this_->modes = value;

        emit modesChanged(value);
    }

    TwitchRoomModes const& TwitchRoom::modes() const
    {
        return this_->modes;
    }

    void TwitchRoom::setMod(bool value)
    {
        this_->isMod = value;
    }

    bool TwitchRoom::isMod() const
    {
        return this_->isMod;
    }

    void TwitchRoom::addChatter(const QString&)
    {
        // TODO: implement
    }

    void TwitchRoom::removeChatter(const QString&)
    {
        // TODO: implement
    }

    MessageContainer& TwitchRoom::messages()
    {
        return *this_->messages;
    }

    QJsonObject TwitchRoom::serialize() const
    {
        return {{"channel", this_->name}};
    }

    QWidget* TwitchRoom::createView(QWidget* parent) const
    {
        auto view = new ui::ChannelView(parent);
        view->setContainer(this_->messages);
        return view;
    }

    void TwitchRoom::fillDropdown(ui::Dropdown& dropdown) const
    {
        dropdown.addItem("Open in Browser", [this]() {
            QDesktopServices::openUrl("https://www.twitch.tv/" + this_->name);
        });
        dropdown.addItem("Open Player in Browser", [this]() {
            QDesktopServices::openUrl(
                "https://player.twitch.tv/?channel=" + this_->name);
        });
        dropdown.addItem("Open in Streamlink", []() { assert(false); });
    }

    // Mentions
    TwitchMentionsRoom::TwitchMentionsRoom()
        : messages_(new MessageContainer)
    {
        this->setTitle("Mentions");
    }

    QJsonObject TwitchMentionsRoom::serialize() const
    {
        return {{"type", "mentions"}};
    }

    QWidget* TwitchMentionsRoom::createView(QWidget* parent) const
    {
        auto view = new ui::ChannelView(parent);
        view->setContainer(this->messages_);
        return view;
    }

    void TwitchMentionsRoom::fillDropdown(ui::Dropdown& dropdown) const
    {
        dropdown.addItem("Mentions dropdown", []() {});
    }

    // Whispers
    TwitchWhispersRoom::TwitchWhispersRoom()
        : messages_(new MessageContainer)
    {
        this->setTitle("Whispers");
    }

    QJsonObject TwitchWhispersRoom::serialize() const
    {
        return {{"type", "whispers"}};
    }

    QWidget* TwitchWhispersRoom::createView(QWidget* parent) const
    {
        auto view = new ui::ChannelView(parent);
        view->setContainer(this->messages_);
        return view;
    }

    void TwitchWhispersRoom::fillDropdown(ui::Dropdown& dropdown) const
    {
        dropdown.addItem("Whispers dropdown", []() {});
    }
}  // namespace chatterino
