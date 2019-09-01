#include "SettingsPage.hpp"

#include "Application.hpp"
#include "singletons/WindowManager.hpp"

#include <QDebug>
#include <QPainter>
#include <util/FunctionEventFilter.hpp>

namespace chatterino {

void filterItemsRec(QObject *object, const QString &query)
{
    for (auto &&child : object->children())
    {
        auto setOpacity = [=](auto *widget, bool condition) {
            widget->greyedOut = !condition;
            widget->update();
        };

        if (auto x = dynamic_cast<SCheckBox *>(child); x)
        {
            setOpacity(x, x->text().contains(query, Qt::CaseInsensitive));
        }
        else if (auto x = dynamic_cast<SLabel *>(child); x)
        {
            setOpacity(x, x->text().contains(query, Qt::CaseInsensitive));
        }
        else if (auto x = dynamic_cast<SComboBox *>(child); x)
        {
            setOpacity(x, [=]() {
                for (int i = 0; i < x->count(); i++)
                {
                    if (x->itemText(i).contains(query, Qt::CaseInsensitive))
                        return true;
                }
                return false;
            }());
        }
        else
        {
            filterItemsRec(child, query);
        }
    }
}

SettingsPage::SettingsPage(const QString &name, const QString &iconResource)
    : name_(name)
    , iconResource_(iconResource)
{
}

void SettingsPage::filterElements(const QString &query)
{
    filterItemsRec(this, query);
}

const QString &SettingsPage::getName()
{
    return this->name_;
}

const QString &SettingsPage::getIconResource()
{
    return this->iconResource_;
}

SettingsDialogTab *SettingsPage::tab() const
{
    return this->tab_;
}

void SettingsPage::setTab(SettingsDialogTab *tab)
{
    this->tab_ = tab;
}

void SettingsPage::cancel()
{
    this->onCancel_.invoke();
}

QCheckBox *SettingsPage::createCheckBox(
    const QString &text, pajlada::Settings::Setting<bool> &setting)
{
    QCheckBox *checkbox = new SCheckBox(text);

    // update when setting changes
    setting.connect(
        [checkbox](const bool &value, auto) {
            checkbox->setChecked(value);  //
        },
        this->managedConnections_);

    // update setting on toggle
    QObject::connect(checkbox, &QCheckBox::toggled, this,
                     [&setting](bool state) {
                         setting = state;
                         getApp()->windows->forceLayoutChannelViews();
                     });

    return checkbox;
}

QComboBox *SettingsPage::createComboBox(
    const QStringList &items, pajlada::Settings::Setting<QString> &setting)
{
    QComboBox *combo = new SComboBox();

    // update setting on toogle
    combo->addItems(items);

    // update when setting changes
    setting.connect(
        [combo](const QString &value, auto) { combo->setCurrentText(value); },
        this->managedConnections_);

    QObject::connect(
        combo, &QComboBox::currentTextChanged,
        [&setting](const QString &newValue) { setting = newValue; });

    return combo;
}

QLineEdit *SettingsPage::createLineEdit(
    pajlada::Settings::Setting<QString> &setting)
{
    QLineEdit *edit = new QLineEdit();

    edit->setText(setting);

    // update when setting changes
    QObject::connect(
        edit, &QLineEdit::textChanged,
        [&setting](const QString &newValue) { setting = newValue; });

    return edit;
}

QSpinBox *SettingsPage::createSpinBox(pajlada::Settings::Setting<int> &setting,
                                      int min, int max)
{
    QSpinBox *w = new QSpinBox;

    w->setMinimum(min);
    w->setMaximum(max);

    setting.connect([w](const int &value, auto) { w->setValue(value); });
    QObject::connect(w, QOverload<int>::of(&QSpinBox::valueChanged),
                     [&setting](int value) { setting.setValue(value); });

    return w;
}

}  // namespace chatterino
