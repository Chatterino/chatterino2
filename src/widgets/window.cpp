#include "widgets/window.hpp"

#include "application.hpp"
#include "controllers/accounts/accountcontroller.hpp"
#include "singletons/ircmanager.hpp"
#include "singletons/settingsmanager.hpp"
#include "singletons/thememanager.hpp"
#include "singletons/windowmanager.hpp"
#include "widgets/accountswitchpopupwidget.hpp"
#include "widgets/helper/shortcut.hpp"
#include "widgets/notebook.hpp"
#include "widgets/settingsdialog.hpp"
#include "widgets/split.hpp"

#include <QApplication>
#include <QHeaderView>
#include <QPalette>
#include <QShortcut>
#include <QVBoxLayout>

#include <QStandardItemModel>

namespace chatterino {
namespace widgets {

Window::Window(WindowType _type)
    : BaseWindow(nullptr, BaseWindow::EnableCustomFrame)
    , type(_type)
    , dpi(this->getScale())
    , notebook(this)
{
    auto app = getApp();

    app->accounts->twitch.currentUserChanged.connect([this] {
        if (this->userLabel == nullptr) {
            return;
        }

        auto user = getApp()->accounts->twitch.getCurrent();

        if (user->isAnon()) {
            this->refreshWindowTitle("Not logged in");

            this->userLabel->getLabel().setText("anonymous");
        } else {
            this->refreshWindowTitle(user->getUserName());

            this->userLabel->getLabel().setText(user->getUserName());
        }
    });

    if (this->hasCustomWindowFrame() && _type == Window::Main) {
        this->addTitleBarButton(TitleBarButton::Settings, [app] {
            app->windows->showSettingsDialog();  //
        });

        this->userLabel = this->addTitleBarLabel([this, app] {
            app->windows->showAccountSelectPopup(
                this->userLabel->mapToGlobal(this->userLabel->rect().bottomLeft()));  //
        });
    }

    if (_type == Window::Main) {
        this->resize(int(600 * this->getScale()), int(500 * this->getScale()));
    } else {
        this->resize(int(300 * this->getScale()), int(500 * this->getScale()));
    }

    QVBoxLayout *layout = new QVBoxLayout(this);

    layout->addWidget(&this->notebook);
    this->getLayoutContainer()->setLayout(layout);

    // set margin
    layout->setMargin(0);

    /// Initialize program-wide hotkeys
    // CTRL+P: Open settings dialog
    CreateWindowShortcut(this, "CTRL+P", [] { SettingsDialog::showDialog(); });

    // CTRL+T: Create new split
    CreateWindowShortcut(this, "CTRL+T",
                         [this] { this->notebook.getOrAddSelectedPage()->appendNewSplit(true); });

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
    CreateWindowShortcut(this, "CTRL+SHIFT+T", [this] { this->notebook.addPage(true); });

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

    //    CreateWindowShortcut(this, "F5", [cheerMessages] {
    //        auto &ircManager = singletons::IrcManager::getInstance();
    //        static int index = 0;
    //        ircManager.addFakeMessage(cheerMessages[index++ % cheerMessages.size()]);
    //    });

    this->setWindowTitle("Chatterino 2 Development Build");

    this->notebook.setAllowUserTabManagement(true);
    this->notebook.setShowAddButton(true);
}

Window::WindowType Window::getType()
{
    return this->type;
}

void Window::repaintVisibleChatWidgets(Channel *channel)
{
    auto *page = this->notebook.getOrAddSelectedPage();

    if (page == nullptr) {
        return;
    }

    for (const auto &split : page->getSplits()) {
        if (channel == nullptr || channel == split->getChannel().get()) {
            split->layoutMessages();
        }
    }
}

SplitNotebook &Window::getNotebook()
{
    return this->notebook;
}

void Window::refreshWindowTitle(const QString &username)
{
    this->setWindowTitle(username + " - Chatterino for Twitch");
}

bool Window::event(QEvent *event)
{
    switch (event->type()) {
        case QEvent::WindowActivate:
            break;

        case QEvent::WindowDeactivate: {
            auto page = this->notebook.getOrAddSelectedPage();

            if (page != nullptr) {
                std::vector<Split *> splits = page->getSplits();

                for (Split *split : splits) {
                    split->updateLastReadMessage();
                }
            }

            if (SplitContainer *container = dynamic_cast<SplitContainer *>(page)) {
                container->hideResizeHandles();
            }
        } break;

        default:;
    };
    return BaseWindow::event(event);
}

void Window::closeEvent(QCloseEvent *)
{
    if (this->type == Window::Main) {
        auto app = getApp();
        app->windows->save();
        app->windows->closeAll();
    }

    this->closed.invoke();

    if (this->type == Window::Main) {
        QApplication::exit();
    }
}

}  // namespace widgets
}  // namespace chatterino
