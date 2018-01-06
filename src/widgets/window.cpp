#include "widgets/window.hpp"
#include "singletons/accountmanager.hpp"
#include "singletons/channelmanager.hpp"
#include "singletons/settingsmanager.hpp"
#include "singletons/thememanager.hpp"
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
    : BaseWidget(_themeManager, nullptr)
    , settingRoot(fS("/windows/{}", windowName))
    , windowGeometry(this->settingRoot)
    , dpi(this->getDpiMultiplier())
    , themeManager(_themeManager)
    , notebook(this, _isMainWindow, this->settingRoot)
{
    this->initAsWindow();

    singletons::AccountManager::getInstance().Twitch.currentUsername.connect(
        [this](const std::string &newUsername, auto) {
            if (newUsername.empty()) {
                this->refreshWindowTitle("Not logged in");
            } else {
                this->refreshWindowTitle(QString::fromStdString(newUsername));
            }
        });

    QVBoxLayout *layout = new QVBoxLayout(this);

    // add titlebar
    //    if (SettingsManager::getInstance().useCustomWindowFrame.get()) {
    //        layout->addWidget(&_titleBar);
    //    }

    layout->addWidget(&this->notebook);
    setLayout(layout);

    // set margin
    //    if (SettingsManager::getInstance().useCustomWindowFrame.get()) {
    //        layout->setMargin(1);
    //    } else {
    layout->setMargin(0);
    //    }

    this->refreshTheme();

    this->loadGeometry();

    /// Initialize program-wide hotkeys
    // CTRL+P: Open Settings Dialog
    CreateWindowShortcut(this, "CTRL+P", [] {
        SettingsDialog::showDialog();  //
    });

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
}

void Window::repaintVisibleChatWidgets(Channel *channel)
{
    auto *page = this->notebook.getSelectedPage();

    if (page == nullptr) {
        return;
    }

    const std::vector<Split *> &widgets = page->getChatWidgets();

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

void Window::refreshTheme()
{
    QPalette palette;
    palette.setColor(QPalette::Background,
                     this->themeManager.tabs.regular.backgrounds.regular.color());
    this->setPalette(palette);
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
