#pragma once

#include "Application.hpp"
#include "common/ChatterinoSetting.hpp"
#include "singletons/WindowManager.hpp"
#include "widgets/helper/SignalLabel.hpp"

#include <boost/variant.hpp>
#include <pajlada/signals/signalholder.hpp>
#include <QCheckBox>
#include <QComboBox>
#include <QDebug>
#include <QPushButton>
#include <QSpinBox>
#include <QVBoxLayout>

#include <utility>

class QScrollArea;

namespace chatterino {

class ColorButton;

class Space : public QLabel
{
    Q_OBJECT
};

class TitleLabel : public QLabel
{
    Q_OBJECT

public:
    TitleLabel(const QString &text)
        : QLabel(text)
    {
    }
};

class SubtitleLabel : public QLabel
{
    Q_OBJECT

public:
    SubtitleLabel(const QString &text)
        : QLabel(text)
    {
    }
};

class NavigationLabel : public SignalLabel
{
    Q_OBJECT

public:
    NavigationLabel(const QString &text)
        : SignalLabel()
    {
        this->setText(text);
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

class ComboBox : public QComboBox
{
    Q_OBJECT

    void wheelEvent(QWheelEvent *event) override
    {
    }
};

struct DropdownArgs {
    QString value;
    int index;
    QComboBox *combobox;
};

class GeneralPageView : public QWidget
{
    Q_OBJECT

public:
    GeneralPageView(QWidget *parent = nullptr);

    void addWidget(QWidget *widget);
    void addLayout(QLayout *layout);
    void addStretch();

    TitleLabel *addTitle(const QString &text);
    SubtitleLabel *addSubtitle(const QString &text);
    /// @param inverse Inverses true to false and vice versa
    QCheckBox *addCheckbox(const QString &text, BoolSetting &setting,
                           bool inverse = false, QString toolTipText = {});
    ComboBox *addDropdown(const QString &text, const QStringList &items,
                          QString toolTipText = {});
    ComboBox *addDropdown(const QString &text, const QStringList &items,
                          pajlada::Settings::Setting<QString> &setting,
                          bool editable = false, QString toolTipText = {});
    ColorButton *addColorButton(const QString &text, const QColor &color,
                                pajlada::Settings::Setting<QString> &setting,
                                QString toolTipText = {});
    QSpinBox *addIntInput(const QString &text, IntSetting &setting, int min,
                          int max, int step, QString toolTipText = {});
    void addNavigationSpacing();

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
        std::function<T(DropdownArgs)> setValue, bool editable = true,
        QString toolTipText = {})
    {
        auto items2 = items;
        auto selected = getValue(setting.getValue());

        if (selected.which() == 1)
        {
            // QString
            if (!editable && !items2.contains(boost::get<QString>(selected)))
                items2.insert(0, boost::get<QString>(selected));
        }

        auto combo = this->addDropdown(text, items2, toolTipText);
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
            combo, QOverload<const int>::of(&QComboBox::currentIndexChanged),
            [combo, &setting,
             setValue = std::move(setValue)](const int newIndex) {
                setting = setValue(DropdownArgs{combo->itemText(newIndex),
                                                combo->currentIndex(), combo});
                getApp()->windows->forceLayoutChannelViews();
            });

        return combo;
    }

    template <typename T>
    ComboBox *addDropdown(
        const QString &text,
        const std::vector<std::pair<QString, QVariant>> &items,
        pajlada::Settings::Setting<T> &setting,
        std::function<boost::variant<int, QString>(ComboBox *, T)> getValue,
        std::function<T(DropdownArgs)> setValue, QString toolTipText = {},
        const QString &defaultValueText = {})
    {
        auto *combo = this->addDropdown(text, {}, std::move(toolTipText));

        for (const auto &[text, userData] : items)
        {
            combo->addItem(text, userData);
        }

        if (!defaultValueText.isEmpty())
        {
            combo->setCurrentText(defaultValueText);
        }

        setting.connect(
            [getValue = std::move(getValue), combo](const T &value, auto) {
                auto var = getValue(combo, value);
                if (var.which() == 0)
                {
                    const auto index = boost::get<int>(var);
                    if (index >= 0)
                    {
                        combo->setCurrentIndex(index);
                    }
                }
                else
                {
                    combo->setCurrentText(boost::get<QString>(var));
                    combo->setEditText(boost::get<QString>(var));
                }
            },
            this->managedConnections_);

        QObject::connect(
            combo, QOverload<const int>::of(&QComboBox::currentIndexChanged),
            [combo, &setting,
             setValue = std::move(setValue)](const int newIndex) {
                setting = setValue(DropdownArgs{combo->itemText(newIndex),
                                                combo->currentIndex(), combo});
                getApp()->windows->forceLayoutChannelViews();
            });

        return combo;
    }

    DescriptionLabel *addDescription(const QString &text);

    void addSeperator();
    bool filterElements(const QString &query);

protected:
    void resizeEvent(QResizeEvent *ev) override
    {
    }

private:
    void updateNavigationHighlighting();
    void addToolTip(QWidget &widget, QString text) const;

    struct Widget {
        QWidget *element;
        QStringList keywords;
    };

    struct Group {
        QString name;
        QWidget *title{};
        QWidget *navigationLink{};
        Space *space{};
        std::vector<Widget> widgets;
    };

    QScrollArea *contentScrollArea_;
    QVBoxLayout *contentLayout_;
    QVBoxLayout *navigationLayout_;

    std::vector<Group> groups_;
    pajlada::Signals::SignalHolder managedConnections_;
};

}  // namespace chatterino
