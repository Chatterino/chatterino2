#pragma once

#include <QCheckBox>
#include <QComboBox>
#include <QLineEdit>
#include <QSpinBox>
#include <pajlada/signals/signal.hpp>

#include "singletons/Settings.hpp"

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
                color.setAlphaF(0.7);                  \
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

    void cancel();

    QCheckBox *createCheckBox(const QString &text,
                              pajlada::Settings::Setting<bool> &setting);
    QComboBox *createComboBox(const QStringList &items,
                              pajlada::Settings::Setting<QString> &setting);
    QLineEdit *createLineEdit(pajlada::Settings::Setting<QString> &setting);
    QSpinBox *createSpinBox(pajlada::Settings::Setting<int> &setting,
                            int min = 0, int max = 2500);

    virtual void onShow()
    {
    }

protected:
    SettingsDialogTab *tab_;
    pajlada::Signals::NoArgSignal onCancel_;
    std::vector<pajlada::Signals::ScopedConnection> managedConnections_;
};

}  // namespace chatterino
