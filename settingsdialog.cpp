#include "settingsdialog.h"
#include "settingsdialogtab.h"

#include "QPalette"
#include "QFile"
#include "QResource"
#include "QLabel"
#include "QFormLayout"
#include "QComboBox"

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
    buttonBox.addButton(&cancelButton, QDialogButtonBox::ButtonRole::RejectRole);
    okButton.setText("OK");
    cancelButton.setText("Cancel");

    resize(600, 500);

    addTabs();
}

void SettingsDialog::addTabs()
{
    QVBoxLayout* vbox;

    // Appearance
    vbox = new QVBoxLayout();

    {
        auto form = new QFormLayout();
        auto combo = new QComboBox();
        auto slider = new QSlider(Qt::Horizontal);
        auto font = new QPushButton("select");

        form->addRow("Theme:", combo);
        form->addRow("Theme Hue:", slider);
        form->addRow("Font:", font);

        vbox->addLayout(form);
    }

    vbox->addStretch(1);

    addTab(vbox, "Appearance", ":/images/AppearanceEditorPart_16x.png");

    // Commands
    vbox = new QVBoxLayout();

    vbox->addWidget(new QLabel());

    vbox->addStretch(1);

    addTab(vbox, "Commands", ":/images/CustomActionEditor_16x.png");

    // Add stretch
    tabs.addStretch(1);
}

void SettingsDialog::addTab(QLayout* layout, QString title, QString imageRes)
{
    auto widget = new QWidget();

    widget->setLayout(layout);

    auto tab = new SettingsDialogTab(this, title, imageRes);

    tab->widget = widget;

    tabs.addWidget(tab, 0, Qt::AlignTop);

    pageStack.addWidget(widget);

    if (tabs.count() == 1)
    {
        select(tab);
    }
}

void SettingsDialog::select(SettingsDialogTab* tab)
{
    pageStack.setCurrentWidget(tab->widget);

    if (selectedTab != NULL)
    {
        selectedTab->setSelected(false);
        selectedTab->setStyleSheet("");
    }

    tab->setSelected(true);
    tab->setStyleSheet("background: #F00;"
            "background: qlineargradient( x1:0 y1:0, x2:1 y2:0, stop:0 #333, stop:1 #555);"
            "border-right: none;");
    selectedTab = tab;
}
