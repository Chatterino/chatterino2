#pragma once

#include <pajlada/settings.hpp>
#include <pajlada/signals/signal.hpp>
#include <QCheckBox>
#include <QComboBox>
#include <QLabel>
#include <QLineEdit>
#include <QPainter>
#include <QPushButton>
#include <QSpinBox>

#define SETTINGS_PAGE_WIDGET_BOILERPLATE(type, parent) \
    class type : public parent                         \
    {                                                  \
        using parent::parent;                          \
                                                       \
    public:                                            \
        bool greyedOut{};                              \
                                                       \
    protected:                                         \
        void paintEvent(QPaintEvent *e) override       \
        {                                              \
            parent::paintEvent(e);                     \
                                                       \
            if (this->greyedOut)                       \
            {                                          \
                QPainter painter(this);                \
                QColor color = QColor("#222222");      \
                color.setAlphaF(0.7F);                 \
                painter.fillRect(this->rect(), color); \
            }                                          \
        }                                              \
    };

namespace chatterino {

// S* widgets are the same as their Q* counterparts,
// but they can be greyed out and will be if you search.
SETTINGS_PAGE_WIDGET_BOILERPLATE(SCheckBox, QCheckBox)
SETTINGS_PAGE_WIDGET_BOILERPLATE(SLabel, QLabel)
SETTINGS_PAGE_WIDGET_BOILERPLATE(SComboBox, QComboBox)
SETTINGS_PAGE_WIDGET_BOILERPLATE(SPushButton, QPushButton)

class SettingsDialogTab;

class SettingsPage : public QFrame
{
    Q_OBJECT

public:
    SettingsPage();

    virtual bool filterElements(const QString &query);

    SettingsDialogTab *tab() const;
    void setTab(SettingsDialogTab *tab);

    QCheckBox *createCheckBox(const QString &text,
                              pajlada::Settings::Setting<bool> &setting,
                              const QString &toolTipText = {});
    QComboBox *createComboBox(const QStringList &items,
                              pajlada::Settings::Setting<QString> &setting);
    QLineEdit *createLineEdit(pajlada::Settings::Setting<QString> &setting);
    QSpinBox *createSpinBox(pajlada::Settings::Setting<int> &setting,
                            int min = 0, int max = 2500);
    template <typename T>
    SLabel *createLabel(const std::function<QString(const T &)> &makeText,
                        pajlada::Settings::Setting<T> &setting)
    {
        auto *label = new SLabel();

        setting.connect(
            [label, makeText](const T &value, auto) {
                label->setText(makeText(value));
            },
            this->managedConnections_);

        return label;
    }

    virtual void onShow()
    {
    }

protected:
    SettingsDialogTab *tab_{};
    pajlada::Signals::SignalHolder managedConnections_;
};

}  // namespace chatterino
