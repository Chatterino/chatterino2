#include "widgets/window.hpp"
#include "singletons/accountmanager.hpp"
#include "singletons/channelmanager.hpp"
#include "singletons/ircmanager.hpp"
#include "singletons/settingsmanager.hpp"
#include "singletons/thememanager.hpp"
#include "singletons/windowmanager.hpp"
#include "widgets/accountswitchpopupwidget.hpp"
#include "widgets/helper/shortcut.hpp"
#include "widgets/notebook.hpp"
#include "widgets/settingsdialog.hpp"
#include "widgets/split.hpp"

#include <QPalette>
#include <QShortcut>
#include <QVBoxLayout>

namespace chatterino {
namespace widgets {

Window::Window(const QString &windowName, singletons::ThemeManager &_themeManager,
               bool _isMainWindow)
    : BaseWindow(_themeManager, nullptr, true)
    , settingRoot(fS("/windows/{}", windowName))
    , windowGeometry(this->settingRoot)
    , dpi(this->getScale())
    , themeManager(_themeManager)
    , notebook(this, _isMainWindow, this->settingRoot)
{
    singletons::AccountManager::getInstance().Twitch.currentUsername.connect(
        [this](const std::string &newUsername, auto) {
            if (newUsername.empty()) {
                this->refreshWindowTitle("Not logged in");
            } else {
                this->refreshWindowTitle(QString::fromStdString(newUsername));
            }
        });

    if (this->hasCustomWindowFrame()) {
        this->addTitleBarButton(TitleBarButton::Settings, [] {
            singletons::WindowManager::getInstance().showSettingsDialog();
        });
        this->addTitleBarButton(TitleBarButton::User, [this] {
            singletons::WindowManager::getInstance().showAccountSelectPopup(QCursor::pos());
        });
    }

    QVBoxLayout *layout = new QVBoxLayout(this);

    layout->addWidget(&this->notebook);
    this->getLayoutContainer()->setLayout(layout);

    // set margin
    layout->setMargin(0);

    this->themeRefreshEvent();

    this->loadGeometry();

    /// Initialize program-wide hotkeys
    // CTRL+P: Open Settings Dialog
    CreateWindowShortcut(this, "CTRL+P", [] { SettingsDialog::showDialog(); });

    // CTRL+Number: Switch to n'th tab
    CreateWindowShortcut(this, "CTRL+1", [this] { this->notebook.selectIndex(0); });
    CreateWindowShortcut(this, "CTRL+2", [this] { this->notebook.selectIndex(1); });
    CreateWindowShortcut(this, "CTRL+3", [this] { this->notebook.selectIndex(2); });
    CreateWindowShortcut(this, "CTRL+4", [this] { this->notebook.selectIndex(3); });
    CreateWindowShortcut(this, "CTRL+5", [this] { this->notebook.selectIndex(4); });
    CreateWindowShortcut(this, "CTRL+6", [this] { this->notebook.selectIndex(5); });
    CreateWindowShortcut(this, "CTRL+7", [this] { this->notebook.selectIndex(6); });
    CreateWindowShortcut(this, "CTRL+8", [this] { this->notebook.selectIndex(7); });
    CreateWindowShortcut(this, "CTRL+9", [this] { this->notebook.selectIndex(8); });

    // CTRL+SHIFT+T: New tab
    CreateWindowShortcut(this, "CTRL+SHIFT+T", [this] { this->notebook.addNewPage(); });

    // CTRL+SHIFT+W: Close current tab
    CreateWindowShortcut(this, "CTRL+SHIFT+W", [this] { this->notebook.removeCurrentPage(); });

    std::vector<QString> cheerMessages;
    // clang-format off
    cheerMessages.emplace_back(R"(@badges=subscriber/12,premium/1;bits=2000;color=#B22222;display-name=arzenhuz;emotes=185989:33-37;id=1ae336ac-8e1a-4d6b-8b00-9fcee26e8337;mod=0;room-id=11148817;subscriber=1;tmi-sent-ts=1515783470139;turbo=0;user-id=111553331;user-type= :arzenhuz!arzenhuz@arzenhuz.tmi.twitch.tv PRIVMSG #pajlada :pajacheer2000 Buy pizza for both pajaH)");
    cheerMessages.emplace_back(R"(@badges=subscriber/12,premium/1;bits=37;color=#3FBF72;display-name=VADIKUS007;emotes=;id=eedd95fd-2a17-4da1-879c-a1e76ffce582;mod=0;room-id=11148817;subscriber=1;tmi-sent-ts=1515783184352;turbo=0;user-id=72256775;user-type= :vadikus007!vadikus007@vadikus007.tmi.twitch.tv PRIVMSG #pajlada :cheer37)");
    cheerMessages.emplace_back(R"(@badges=moderator/1,subscriber/24,bits/100;bits=1;color=#DCD3E6;display-name=swiftapples;emotes=80803:7-13;id=1c4647f6-f1a8-4acc-a9b2-b5d23d91258d;mod=1;room-id=11148817;subscriber=1;tmi-sent-ts=1515538318854;turbo=0;user-id=80526177;user-type=mod :swiftapples!swiftapples@swiftapples.tmi.twitch.tv PRIVMSG #pajlada :cheer1 pajaHey)");
    cheerMessages.emplace_back(R"(@badges=subscriber/12,turbo/1;bits=1;color=#0A2927;display-name=Binkelderk;emotes=;id=a1d9bdc6-6f6a-4c03-8554-d5b34721a878;mod=0;room-id=11148817;subscriber=1;tmi-sent-ts=1515538899479;turbo=1;user-id=89081828;user-type= :binkelderk!binkelderk@binkelderk.tmi.twitch.tv PRIVMSG #pajlada :pajacheer1)");
    cheerMessages.emplace_back(R"(@badges=moderator/1,subscriber/24,bits/100;bits=1;color=#DCD3E6;display-name=swiftapples;emotes=80803:6-12;id=e9e21793-0b58-4ac6-8a1e-c19e165dbc9f;mod=1;room-id=11148817;subscriber=1;tmi-sent-ts=1515539073209;turbo=0;user-id=80526177;user-type=mod :swiftapples!swiftapples@swiftapples.tmi.twitch.tv PRIVMSG #pajlada :bday1 pajaHey)");
    cheerMessages.emplace_back(R"(@badges=partner/1;bits=1;color=#CC44FF;display-name=pajlada;emotes=;id=ca89214e-4fb5-48ec-853e-d2e6b41355ea;mod=0;room-id=39705480;subscriber=0;tmi-sent-ts=1515546977622;turbo=0;user-id=11148817;user-type= :pajlada!pajlada@pajlada.tmi.twitch.tv PRIVMSG #leesherwhy :test123 owocheer1 456test)");
    cheerMessages.emplace_back(R"(@badges=subscriber/12,premium/1;bits=1;color=#3FBF72;display-name=VADIKUS007;emotes=;id=c4c5061b-f5c6-464b-8bff-7f1ac816caa7;mod=0;room-id=11148817;subscriber=1;tmi-sent-ts=1515782817171;turbo=0;user-id=72256775;user-type= :vadikus007!vadikus007@vadikus007.tmi.twitch.tv PRIVMSG #pajlada :trihard1)");
    cheerMessages.emplace_back(R"(@badges=;bits=1;color=#FF0000;display-name=?????;emotes=;id=979b6b4f-be9a-42fb-a54c-88fcb0aca18d;mod=0;room-id=11148817;subscriber=0;tmi-sent-ts=1515782819084;turbo=0;user-id=70656218;user-type= :stels_tv!stels_tv@stels_tv.tmi.twitch.tv PRIVMSG #pajlada :trihard1)");
    cheerMessages.emplace_back(R"(@badges=subscriber/3,premium/1;bits=1;color=#FF0000;display-name=kalvarenga;emotes=;id=4744d6f0-de1d-475d-a3ff-38647113265a;mod=0;room-id=11148817;subscriber=1;tmi-sent-ts=1515782860740;turbo=0;user-id=108393131;user-type= :kalvarenga!kalvarenga@kalvarenga.tmi.twitch.tv PRIVMSG #pajlada :trihard1)");
    // clang-format on

    CreateWindowShortcut(this, "F5", [cheerMessages] {
        auto &ircManager = singletons::IrcManager::getInstance();
        static int index = 0;
        ircManager.addFakeMessage(cheerMessages[index++ % cheerMessages.size()]);
    });
}

void Window::repaintVisibleChatWidgets(Channel *channel)
{
    auto *page = this->notebook.getSelectedPage();

    if (page == nullptr) {
        return;
    }

    const std::vector<Split *> &widgets = page->getSplits();

    for (auto it = widgets.begin(); it != widgets.end(); ++it) {
        Split *widget = *it;

        if (channel == nullptr || channel == widget->getChannel().get()) {
            widget->layoutMessages();
        }
    }
}

Notebook &Window::getNotebook()
{
    return this->notebook;
}

void Window::refreshWindowTitle(const QString &username)
{
    this->setWindowTitle(username + " - Chatterino for Twitch");
}

void Window::closeEvent(QCloseEvent *)
{
    const QRect &geom = this->geometry();

    this->windowGeometry.x = geom.x();
    this->windowGeometry.y = geom.y();
    this->windowGeometry.width = geom.width();
    this->windowGeometry.height = geom.height();

    this->closed();
}

bool Window::event(QEvent *e)
{
    switch (e->type()) {
        case QEvent::WindowActivate:
            break;

        case QEvent::WindowDeactivate: {
            auto page = this->notebook.getSelectedPage();

            if (page != nullptr) {
                std::vector<Split *> splits = page->getSplits();

                for (Split *split : splits) {
                    split->updateLastReadMessage();
                }
            }
        } break;
    };
    return BaseWindow::event(e);
}

void Window::loadGeometry()
{
    bool doSetGeometry = false;
    QRect loadedGeometry;
    if (!this->windowGeometry.x.isDefaultValue() && !this->windowGeometry.y.isDefaultValue()) {
        loadedGeometry.setX(this->windowGeometry.x);
        loadedGeometry.setY(this->windowGeometry.y);
        doSetGeometry = true;
    }

    if (!this->windowGeometry.width.isDefaultValue() &&
        !this->windowGeometry.height.isDefaultValue()) {
        loadedGeometry.setWidth(this->windowGeometry.width);
        loadedGeometry.setHeight(this->windowGeometry.height);
    } else {
        loadedGeometry.setWidth(1280);
        loadedGeometry.setHeight(720);
    }

    if (doSetGeometry) {
        this->setGeometry(loadedGeometry);
    } else {
        this->resize(loadedGeometry.width(), loadedGeometry.height());
    }
}

void Window::save()
{
    this->notebook.save();
}

}  // namespace widgets
}  // namespace chatterino
