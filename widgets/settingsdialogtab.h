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
    Q_PROPERTY(bool getSelected READ getSelected WRITE setSelected NOTIFY selectedChanged)

public:
    SettingsDialogTab(SettingsDialog *_dialog, QString _label, QString imageRes);

    void setSelected(bool selected);
    bool getSelected() const;
    QWidget *getWidget();
    void setWidget(QWidget *widget);

signals:
    void selectedChanged(bool);

private:
    void paintEvent(QPaintEvent *);
    void mousePressEvent(QMouseEvent *event);

    QWidget *_widget;
    QString _label;
    QImage _image;

    SettingsDialog *_dialog;

    bool _selected;
};
}  // namespace  widgets
}  // namespace  chatterino

#endif  // SETTINGSNOTEBOOKTAB_H
