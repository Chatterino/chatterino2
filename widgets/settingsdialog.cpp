#include "widgets/settingsdialog.h"
#include "widgets/settingsdialogtab.h"

#include <QComboBox>
#include <QFile>
#include <QFormLayout>
#include <QGroupBox>
#include <QLabel>
#include <QPalette>
#include <QResource>

namespace chatterino {
namespace widgets {

SettingsDialog::SettingsDialog()
{
    QFile file(":/qss/settings.qss");
    file.open(QFile::ReadOnly);
    QString styleSheet = QLatin1String(file.readAll());
    setStyleSheet(styleSheet);

    QPalette palette;
    palette.setColor(QPalette::Background, QColor("#444"));
    setPalette(palette);

    pageStack.setObjectName("pages");

    setLayout(&vbox);

    vbox.addLayout(&hbox);

    vbox.addWidget(&buttonBox);

    auto tabWidget = new QWidget();
    tabWidget->setObjectName("tabWidget");

    tabWidget->setLayout(&tabs);
    tabWidget->setFixedWidth(200);

    hbox.addWidget(tabWidget);
    hbox.addLayout(&pageStack);

    buttonBox.addButton(&okButton, QDialogButtonBox::ButtonRole::AcceptRole);
    buttonBox.addButton(&cancelButton,
                        QDialogButtonBox::ButtonRole::RejectRole);
    okButton.setText("OK");
    cancelButton.setText("Cancel");

    resize(600, 500);

    addTabs();
}

void
SettingsDialog::addTabs()
{
    QVBoxLayout *vbox;

    // Appearance
    vbox = new QVBoxLayout();

    {
        auto group = new QGroupBox("Application");

        auto form = new QFormLayout();
        auto combo = new QComboBox();
        auto slider = new QSlider(Qt::Horizontal);
        auto font = new QPushButton("select");

        form->addRow("Theme:", combo);
        form->addRow("Theme color:", slider);
        form->addRow("Font:", font);

        group->setLayout(form);

        vbox->addWidget(group);
    }

    {
        auto group = new QGroupBox("Messages");

        auto v = new QVBoxLayout();
        v->addWidget(createCheckbox("Show timestamp", ""));
        v->addWidget(createCheckbox("Show seconds in timestamp", ""));
        v->addWidget(createCheckbox(
            "Allow sending duplicate messages (add a space at the end)", ""));
        v->addWidget(createCheckbox("Seperate messages", ""));
        v->addWidget(createCheckbox("Show message length", ""));

        group->setLayout(v);

        vbox->addWidget(group);
    }

    vbox->addStretch(1);

    addTab(vbox, "Appearance", ":/images/AppearanceEditorPart_16x.png");

    // Behaviour
    vbox = new QVBoxLayout();

    vbox->addWidget(createCheckbox("Hide input box if empty", ""));
    vbox->addWidget(
        createCheckbox("Mention users with a @ (except in commands)", ""));
    vbox->addWidget(createCheckbox("Window always on top", ""));
    vbox->addWidget(createCheckbox("Show last read message indicator", ""));

    {
        auto v = new QVBoxLayout();
        v->addWidget(new QLabel("Mouse scroll speed"));

        auto scroll = new QSlider(Qt::Horizontal);

        v->addWidget(scroll);
        v->addStretch(1);
    }

    vbox->addStretch(1);

    addTab(vbox, "Behaviour", ":/images/AppearanceEditorPart_16x.png");

    // Commands
    vbox = new QVBoxLayout();

    vbox->addWidget(new QLabel());

    vbox->addStretch(1);

    addTab(vbox, "Commands", ":/images/CustomActionEditor_16x.png");

    // Add stretch
    tabs.addStretch(1);
}

QCheckBox *
SettingsDialog::createCheckbox(QString title, QString settingsId)
{
    return new QCheckBox(title);
}

void
SettingsDialog::addTab(QLayout *layout, QString title, QString imageRes)
{
    auto widget = new QWidget();

    widget->setLayout(layout);

    auto tab = new SettingsDialogTab(this, title, imageRes);

    tab->setWidget(widget);

    tabs.addWidget(tab, 0, Qt::AlignTop);

    pageStack.addWidget(widget);

    if (tabs.count() == 1) {
        select(tab);
    }
}

void
SettingsDialog::select(SettingsDialogTab *tab)
{
    pageStack.setCurrentWidget(tab->getWidget());

    if (selectedTab != NULL) {
        selectedTab->setSelected(false);
        selectedTab->setStyleSheet("");
    }

    tab->setSelected(true);
    tab->setStyleSheet("background: #F00;"
                       "background: qlineargradient( x1:0 y1:0, x2:1 y2:0, "
                       "stop:0 #333, stop:1 #555);"
                       "border-right: none;");
    selectedTab = tab;
}
}
}
