#ifndef SETTINGSNOTEBOOKTAB_H
#define SETTINGSNOTEBOOKTAB_H

#include <QWidget>
#include "QPaintEvent"

class SettingsDialog;

class SettingsDialogTab : public QWidget
{
    Q_OBJECT
    Q_PROPERTY(
        bool selected READ selected WRITE setSelected NOTIFY selectedChanged)

public:
    SettingsDialogTab(SettingsDialog *dialog, QString label, QString imageRes);

    void
    setSelected(bool selected)
    {
        if (selected == m_selected)
            return;

        m_selected = selected;
        emit selectedChanged(selected);
    }

    bool
    selected() const
    {
        return m_selected;
    }

    QWidget *widget;

signals:
    void selectedChanged(bool);

private:
    void paintEvent(QPaintEvent *);
    void mouseReleaseEvent(QMouseEvent *event);

    QString label;
    QImage image;

    SettingsDialog *dialog = NULL;

    bool m_selected = false;
};

#endif  // SETTINGSNOTEBOOKTAB_H
