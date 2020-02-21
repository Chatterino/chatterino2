#pragma once

#include "widgets/BaseWidget.hpp"

#include <QIcon>
#include <QPaintEvent>
#include <QWidget>

namespace chatterino {

class SettingsPage;
class SettingsDialog;

enum SettingsTabId {
    None,
    Accounts,
    Moderation,
};

class SettingsDialogTab : public BaseWidget
{
    Q_OBJECT

public:
    SettingsDialogTab(SettingsDialog *dialog_, SettingsPage *page_,
                      QString imageFileName, SettingsTabId id = {});

    void setSelected(bool selected_);
    SettingsPage *page();
    SettingsTabId id() const;

signals:
    void selectedChanged(bool);

private:
    void paintEvent(QPaintEvent *);
    void mousePressEvent(QMouseEvent *event);

    struct {
        QString labelText;
        QIcon icon;
    } ui_;

    // Parent settings dialog
    SettingsDialog *dialog_;
    SettingsPage *page_;
    SettingsTabId id_;

    bool selected_ = false;
};

}  // namespace chatterino
