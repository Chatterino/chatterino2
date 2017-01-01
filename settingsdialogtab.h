#ifndef SETTINGSNOTEBOOKTAB_H
#define SETTINGSNOTEBOOKTAB_H

#include <QWidget>
#include "QPaintEvent"

class SettingsDialogTab : public QWidget
{
    Q_OBJECT
public:
    SettingsDialogTab(QWidget *parent, QString label, QImage& image);

private:
    void paintEvent(QPaintEvent *);
    QString label;
    QImage& image;
};

#endif // SETTINGSNOTEBOOKTAB_H
