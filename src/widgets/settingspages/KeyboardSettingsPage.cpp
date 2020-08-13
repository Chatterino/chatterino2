#include "KeyboardSettingsPage.hpp"

#include "util/LayoutCreator.hpp"

#include <QFormLayout>
#include <QLabel>

namespace chatterino {

KeyboardSettingsPage::KeyboardSettingsPage()
{
    auto layout =
        LayoutCreator<KeyboardSettingsPage>(this).setLayoutType<QVBoxLayout>();

    auto scroll = layout.emplace<QScrollArea>();

    this->setStyleSheet("QLabel, #container { background: #333 }");

    auto form = new QFormLayout(this);
    scroll->setWidgetResizable(true);
    auto widget = new QWidget();
    widget->setLayout(form);
    widget->setObjectName("container");
    scroll->setWidget(widget);

    form->addRow(new QLabel("Hold Ctrl"), new QLabel("Show resize handles"));
    form->addRow(new QLabel("Hold Ctrl + Alt"),
                 new QLabel("Show split overlay"));

    form->addItem(new QSpacerItem(16, 16));
    form->addRow(new QLabel("Ctrl + ScrollDown/-"), new QLabel("Zoom out"));
    form->addRow(new QLabel("Ctrl + ScrollUp/+"), new QLabel("Zoom in"));
    form->addRow(new QLabel("Ctrl + 0"), new QLabel("Reset zoom size"));

    form->addItem(new QSpacerItem(16, 16));
    form->addRow(new QLabel("Ctrl + T"), new QLabel("Create new split"));
    form->addRow(new QLabel("Ctrl + W"), new QLabel("Close current split"));
    form->addRow(new QLabel("Ctrl + N"),
                 new QLabel("Open current split as a popup"));
    form->addRow(new QLabel("Ctrl + K"), new QLabel("Jump to split"));
    form->addRow(new QLabel("Ctrl + G"),
                 new QLabel("Reopen last closed split"));

    form->addRow(new QLabel("Ctrl + Shift + T"), new QLabel("Create new tab"));
    form->addRow(new QLabel("Ctrl + Shift + W"),
                 new QLabel("Close current tab"));
    form->addRow(new QLabel("Ctrl + H"),
                 new QLabel("Hide/Show similar messages (See General->R9K)"));

    form->addItem(new QSpacerItem(16, 16));
    form->addRow(new QLabel("Ctrl + 1/2/3/..."),
                 new QLabel("Select tab 1/2/3/..."));
    form->addRow(new QLabel("Ctrl + 9"), new QLabel("Select last tab"));
    form->addRow(new QLabel("Ctrl + Tab"), new QLabel("Select next tab"));
    form->addRow(new QLabel("Ctrl + Shift + Tab"),
                 new QLabel("Select previous tab"));

    form->addRow(new QLabel("Alt + ←/↑/→/↓"),
                 new QLabel("Select left/upper/right/bottom split"));

    form->addItem(new QSpacerItem(16, 16));
    form->addRow(new QLabel("Ctrl + R"), new QLabel("Change channel"));
    form->addRow(new QLabel("Ctrl + F"),
                 new QLabel("Search in current channel"));
    form->addRow(new QLabel("Ctrl + E"), new QLabel("Open Emote menu"));
    form->addRow(new QLabel("Ctrl + P"), new QLabel("Open Settings menu"));
    form->addRow(new QLabel("F5"),
                 new QLabel("Reload subscriber and channel emotes"));
}

}  // namespace chatterino
