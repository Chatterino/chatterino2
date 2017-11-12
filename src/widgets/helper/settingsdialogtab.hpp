#pragma once

#include <QPaintEvent>
#include <QWidget>

namespace chatterino {
namespace widgets {

class SettingsDialog;

class SettingsDialogTab : public QWidget
{
    Q_OBJECT

public:
    SettingsDialogTab(SettingsDialog *dialog, QString _label, QString imageFileName);

    void setSelected(bool selected);
    QWidget *getWidget();
    void setWidget(QWidget *widget);

signals:
    void selectedChanged(bool);

private:
    void paintEvent(QPaintEvent *);
    void mousePressEvent(QMouseEvent *event);

    struct {
        QWidget *widget;
        QString labelText;
        QImage image;
    } ui;

    // Parent settings dialog
    SettingsDialog *dialog;

    bool selected = false;
};

}  // namespace widgets
}  // namespace chatterino
