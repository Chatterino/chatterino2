#ifndef SETTINGSNOTEBOOKTAB_H
#define SETTINGSNOTEBOOKTAB_H

#include <QWidget>
#include "QPaintEvent"

class SettingsDialog;

class SettingsDialogTab : public QWidget
{
    Q_OBJECT
    Q_PROPERTY(bool getSelected READ getSelected WRITE setSelected NOTIFY
                   selectedChanged)

public:
    SettingsDialogTab(SettingsDialog *dialog, QString label, QString imageRes);

    void
    setSelected(bool selected)
    {
        if (selected == selected)
            return;

        selected = selected;
        emit selectedChanged(selected);
    }

    bool
    getSelected() const
    {
        return selected;
    }

    QWidget *
    getWidget()
    {
        return widget;
    }

    void
    setWidget(QWidget *widget)
    {
        this->widget = widget;
    }

signals:
    void selectedChanged(bool);

private:
    void paintEvent(QPaintEvent *);
    void mouseReleaseEvent(QMouseEvent *event);

    QWidget *widget;
    QString label;
    QImage image;

    SettingsDialog *dialog = NULL;

    bool selected = false;
};

#endif  // SETTINGSNOTEBOOKTAB_H
