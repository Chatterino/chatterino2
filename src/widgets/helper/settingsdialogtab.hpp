#pragma once

#include <QIcon>
#include <QPaintEvent>
#include <QWidget>

namespace chatterino {
namespace widgets {
namespace settingspages {
class SettingsPage;
}

class SettingsDialog;

class SettingsDialogTab : public QWidget
{
    Q_OBJECT

public:
    SettingsDialogTab(SettingsDialog *dialog, settingspages::SettingsPage *page,
                      QString imageFileName);

    void setSelected(bool selected);
    settingspages::SettingsPage *getSettingsPage();

signals:
    void selectedChanged(bool);

private:
    void paintEvent(QPaintEvent *);
    void mousePressEvent(QMouseEvent *event);

    struct {
        QString labelText;
        QIcon icon;
    } ui;

    // Parent settings dialog
    SettingsDialog *dialog;
    settingspages::SettingsPage *page;

    bool selected = false;
};

}  // namespace widgets
}  // namespace chatterino
