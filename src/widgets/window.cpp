#include "widgets/window.hpp"
#include "channelmanager.hpp"
#include "colorscheme.hpp"
#include "settingsmanager.hpp"
#include "widgets/split.hpp"
#include "widgets/notebook.hpp"
#include "widgets/settingsdialog.hpp"

#include <QDebug>
#include <QLibrary>
#include <QPalette>
#include <QShortcut>
#include <QVBoxLayout>
#include <boost/foreach.hpp>

namespace chatterino {
namespace widgets {

Window::Window(ChannelManager &_channelManager, ColorScheme &_colorScheme,
                       CompletionManager &_completionManager, bool _isMainWindow)
    : BaseWidget(_colorScheme, nullptr)
    , channelManager(_channelManager)
    , colorScheme(_colorScheme)
    , completionManager(_completionManager)
    , notebook(this->channelManager, this, _isMainWindow)
    , dpi(this->getDpiMultiplier())
// , windowGeometry("/windows/0/geometry")
{
    this->initAsWindow();

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

    if (/*this->windowGeometry->isFilled()*/ false) {
        // Load geometry from settings file
        // this->setGeometry(this->windowGeometry.getValueRef());
    } else {
        // Set default geometry
        // Default position is in the middle of the current monitor or the primary monitor
        this->resize(1280, 800);
    }

    // Initialize program-wide hotkeys
    {
        // CTRL+P: Open Settings Dialog
        auto shortcut = new QShortcut(QKeySequence("CTRL+P"), this);
        connect(shortcut, &QShortcut::activated, []() {
            SettingsDialog::showDialog();  //
        });
    }
}

Window::~Window()
{
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

void Window::load(const boost::property_tree::ptree &tree)
{
    this->notebook.load(tree);

    loaded = true;
}

boost::property_tree::ptree Window::save()
{
    boost::property_tree::ptree child;

    child.put("type", "main");

    this->notebook.save(child);

    return child;
}

void Window::loadDefaults()
{
    this->notebook.loadDefaults();

    loaded = true;
}

bool Window::isLoaded() const
{
    return loaded;
}

Notebook &Window::getNotebook()
{
    return this->notebook;
}

void Window::closeEvent(QCloseEvent *)
{
    // Save closing window position
    // this->windowGeometry = this->geometry();
}

void Window::refreshTheme()
{
    QPalette palette;
    palette.setColor(QPalette::Background, this->colorScheme.TabBackground);
    this->setPalette(palette);
}

}  // namespace widgets
}  // namespace chatterino
