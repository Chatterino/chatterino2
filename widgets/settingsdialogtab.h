#ifndef SETTINGSNOTEBOOKTAB_H
#define SETTINGSNOTEBOOKTAB_H

#include <QPaintEvent>
#include <QWidget>

namespace chatterino {
namespace widgets {

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
        if (this->selected == selected)
            return;

        this->selected = selected;
        emit selectedChanged(selected);
    }

    bool
    getSelected() const
    {
        return this->selected;
    }

    QWidget *
    getWidget()
    {
        return this->widget;
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
}
}

#endif  // SETTINGSNOTEBOOKTAB_H
