#include "settingspage.hpp"

#include <QDebug>

namespace chatterino {
namespace widgets {
namespace settingspages {

SettingsPage::SettingsPage(const QString &_name, const QString &_iconResource)
    : name(_name)
    , iconResource(_iconResource)
{
}

const QString &SettingsPage::getName()
{
    return this->name;
}

const QString &SettingsPage::getIconResource()
{
    return this->iconResource;
}

void SettingsPage::cancel()
{
    this->onCancel.invoke();
}

QCheckBox *SettingsPage::createCheckBox(const QString &text,
                                        pajlada::Settings::Setting<bool> &setting)
{
    QCheckBox *checkbox = new QCheckBox(text);

    // update when setting changes
    setting.connect(
        [checkbox](const bool &value, auto) {
            checkbox->setChecked(value);  //
        },
        this->managedConnections);

    // update setting on toggle
    QObject::connect(checkbox, &QCheckBox::toggled, this, [&setting](bool state) {
        qDebug() << "update checkbox value";
        setting = state;  //
    });

    return checkbox;
}

QComboBox *SettingsPage::createComboBox(const QStringList &items,
                                        pajlada::Settings::Setting<QString> &setting)
{
    QComboBox *combo = new QComboBox();

    // update setting on toogle
    combo->addItems(items);

    // update when setting changes
    setting.connect([combo](const QString &value, auto) { combo->setCurrentText(value); },
                    this->managedConnections);

    QObject::connect(combo, &QComboBox::currentTextChanged,
                     [&setting](const QString &newValue) { setting = newValue; });

    return combo;
}

QLineEdit *SettingsPage::createLineEdit(pajlada::Settings::Setting<QString> &setting)
{
    QLineEdit *edit = new QLineEdit();

    edit->setText(setting);

    // update when setting changes
    QObject::connect(edit, &QLineEdit::textChanged,
                     [&setting](const QString &newValue) { setting = newValue; });

    return edit;
}

}  // namespace settingspages
}  // namespace widgets
}  // namespace chatterino
