#include "KeyboardSettingsPage.hpp"

#include "util/LayoutCreator.hpp"

#include <QFormLayout>
#include <QLabel>

namespace chatterino {

KeyboardSettingsPage::KeyboardSettingsPage()
    : SettingsPage("Keybindings", ":/settings/keybinds.svg")
{
    auto layout =
        LayoutCreator<KeyboardSettingsPage>(this).setLayoutType<QVBoxLayout>();

    auto form = layout.emplace<QFormLayout>();

    form->addRow(new QLabel("Hold Ctrl"), new QLabel("Show resize handles"));
    form->addRow(new QLabel("Hold Ctrl + Alt"),
                 new QLabel("Show split overlay"));

    form->addItem(new QSpacerItem(16, 16));
    form->addRow(new QLabel("Ctrl + T"), new QLabel("Create new split"));
    form->addRow(new QLabel("Ctrl + W"), new QLabel("Close current split"));

    form->addRow(new QLabel("Ctrl + Shift + T"), new QLabel("Create new tab"));
    form->addRow(new QLabel("Ctrl + Shift + W"),
                 new QLabel("Close current tab"));

    form->addItem(new QSpacerItem(16, 16));
    form->addRow(new QLabel("Ctrl + 1/2/3/..."),
                 new QLabel("Select tab 1/2/3/..."));
    form->addRow(new QLabel("Ctrl + Tab"), new QLabel("Select next tab"));
    form->addRow(new QLabel("Ctrl + Shift + Tab"),
                 new QLabel("Select previous tab"));

    form->addRow(new QLabel("Alt + Left/Up/Right/Down"),
                 new QLabel("Select split left/up/right/down"));

    form->addItem(new QSpacerItem(16, 16));
    form->addRow(new QLabel("Ctrl + R"), new QLabel("Change channel"));
    form->addRow(new QLabel("Ctrl + F"),
                 new QLabel("Search in current channel"));
    form->addRow(new QLabel("Ctrl + E"), new QLabel("Open Emote menu"));
    form->addRow(new QLabel("Ctrl + P"), new QLabel("Open Settings menu"));
}

}  // namespace chatterino
