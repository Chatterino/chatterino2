#pragma once

#include <QDebug>
#include <boost/variant.hpp>
#include "Application.hpp"
#include "common/ChatterinoSetting.hpp"
#include "qlogging.hpp"
#include "singletons/WindowManager.hpp"
#include "widgets/helper/SignalLabel.hpp"

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
                           bool inverse = false);
    ComboBox *addDropdown(const QString &text, const QStringList &items);
    ComboBox *addDropdown(const QString &text, const QStringList &items,
                          pajlada::Settings::Setting<QString> &setting,
                          bool editable = false);
    ColorButton *addColorButton(const QString &text, const QColor &color,
                                pajlada::Settings::Setting<QString> &setting);
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

protected:
    void resizeEvent(QResizeEvent *ev) override
    {
        qCDebug(chatterinoWidget) << ev->size();
    }

private:
    void updateNavigationHighlighting();

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
    std::vector<pajlada::Signals::ScopedConnection> managedConnections_;
};

}  // namespace chatterino
