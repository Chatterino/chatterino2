#include "widgets/window.hpp"
#include "singletons/accountmanager.hpp"
#include "singletons/channelmanager.hpp"
#include "singletons/settingsmanager.hpp"
#include "singletons/thememanager.hpp"
#include "widgets/notebook.hpp"
#include "widgets/settingsdialog.hpp"
#include "widgets/split.hpp"

#include <QPalette>
#include <QShortcut>
#include <QVBoxLayout>

namespace chatterino {
namespace widgets {

Window::Window(const QString &windowName, singletons::ThemeManager &_themeManager, bool _isMainWindow)
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

    // Initialize program-wide hotkeys
    {
        // CTRL+P: Open Settings Dialog
        auto shortcut = new QShortcut(QKeySequence("CTRL+P"), this);
        connect(shortcut, &QShortcut::activated, []() {
            SettingsDialog::showDialog();  //
        });
    }
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
    palette.setColor(QPalette::Background, this->themeManager.TabBackground);
    this->setPalette(palette);
}

void Window::loadGeometry()
{
    if (!this->windowGeometry.x.isDefaultValue() && !this->windowGeometry.y.isDefaultValue()) {
        this->move(this->windowGeometry.x, this->windowGeometry.y);
    }

    if (!this->windowGeometry.width.isDefaultValue() &&
        !this->windowGeometry.height.isDefaultValue()) {
        this->resize(this->windowGeometry.width, this->windowGeometry.height);
    } else {
        this->resize(1280, 800);
    }
}

void Window::save()
{
    this->notebook.save();
}

}  // namespace widgets
}  // namespace chatterino
