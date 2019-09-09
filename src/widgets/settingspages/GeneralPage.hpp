#pragma once

#include <QDebug>
#include <QLabel>
#include <QVBoxLayout>

#include "Application.hpp"
#include "boost/variant.hpp"
#include "pajlada/signals/signal.hpp"
#include "singletons/Settings.hpp"
#include "singletons/WindowManager.hpp"
#include "widgets/settingspages/SettingsPage.hpp"

class QLabel;
class QCheckBox;
class QComboBox;

namespace chatterino {

class TitleLabel : public QLabel
{
    Q_OBJECT

public:
    TitleLabel(const QString &text)
        : QLabel(text)
    {
    }
};

class DescriptionLabel : public QLabel
{
    Q_OBJECT

public:
    DescriptionLabel(const QString &text)
        : QLabel(text)
    {
    }
};

struct DropdownArgs {
    QString value;
    int index;
    QComboBox *combobox;
};

class ComboBox : public QComboBox
{
    Q_OBJECT

    void wheelEvent(QWheelEvent *event) override
    {
    }
};

class SettingsLayout : public QVBoxLayout
{
    Q_OBJECT

public:
    TitleLabel *addTitle(const QString &text);
    /// @param inverse Inverses true to false and vice versa
    QCheckBox *addCheckbox(const QString &text, BoolSetting &setting,
                           bool inverse = false);
    ComboBox *addDropdown(const QString &text, const QStringList &items);
    ComboBox *addDropdown(const QString &text, const QStringList &items,
                          pajlada::Settings::Setting<QString> &setting,
                          bool editable = false);

    template <typename OnClick>
    QPushButton *makeButton(const QString &text, OnClick onClick)
    {
        auto button = new QPushButton(text);
        this->groups_.back().widgets.push_back({button, {text}});
        QObject::connect(button, &QPushButton::clicked, onClick);
        return button;
    }

    template <typename OnClick>
    QPushButton *addButton(const QString &text, OnClick onClick)
    {
        auto button = makeButton(text, onClick);
        auto layout = new QHBoxLayout();
        layout->addWidget(button);
        layout->addStretch(1);
        this->addLayout(layout);
        return button;
    }

    template <typename T>
    ComboBox *addDropdown(
        const QString &text, const QStringList &items,
        pajlada::Settings::Setting<T> &setting,
        std::function<boost::variant<int, QString>(T)> getValue,
        std::function<T(DropdownArgs)> setValue, bool editable = true)
    {
        auto items2 = items;
        auto selected = getValue(setting.getValue());

        if (selected.which() == 1)
        {
            // QString
            if (!editable && !items2.contains(boost::get<QString>(selected)))
                items2.insert(0, boost::get<QString>(selected));
        }

        auto combo = this->addDropdown(text, items2);
        if (editable)
            combo->setEditable(true);

        if (selected.which() == 0)
        {
            // int
            auto value = boost::get<int>(selected);
            if (value >= 0 && value < items2.size())
                combo->setCurrentIndex(value);
        }
        else if (selected.which() == 1)
        {
            // QString
            combo->setEditText(boost::get<QString>(selected));
        }

        setting.connect(
            [getValue = std::move(getValue), combo](const T &value, auto) {
                auto var = getValue(value);
                if (var.which() == 0)
                    combo->setCurrentIndex(boost::get<int>(var));
                else
                {
                    combo->setCurrentText(boost::get<QString>(var));
                    combo->setEditText(boost::get<QString>(var));
                }
            },
            this->managedConnections_);

        QObject::connect(
            combo,
            QOverload<const QString &>::of(&QComboBox::currentIndexChanged),
            //            &QComboBox::editTextChanged,
            [combo, &setting,
             setValue = std::move(setValue)](const QString &newValue) {
                setting = setValue(
                    DropdownArgs{newValue, combo->currentIndex(), combo});
                getApp()->windows->forceLayoutChannelViews();
            });

        return combo;
    }
    DescriptionLabel *addDescription(const QString &text);

    void addSeperator();
    bool filterElements(const QString &query);

private:
    struct Widget {
        QWidget *element;
        QStringList keywords;
    };

    struct Group {
        QString name;
        QWidget *title{};
        std::vector<Widget> widgets;
    };

    std::vector<Group> groups_;
    std::vector<pajlada::Signals::ScopedConnection> managedConnections_;
};

class GeneralPage : public SettingsPage
{
    Q_OBJECT

public:
    GeneralPage();

    bool filterElements(const QString &query);

private:
    void initLayout(SettingsLayout &layout);
    void initExtra();

    QString getFont(const DropdownArgs &args) const;

    DescriptionLabel *cachePath_{};
    SettingsLayout *settingsLayout_{};
};

}  // namespace chatterino
