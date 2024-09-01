#pragma once

#include "widgets/BaseWidget.hpp"

#include <QIcon>
#include <QPaintEvent>
#include <QWidget>

#include <functional>

namespace chatterino {

class SettingsPage;
class SettingsDialog;

enum class SettingsTabId {
    None,
    General,
    Accounts,
    Highlights,
    Moderation,
    About,
};

class SettingsDialogTab : public BaseWidget
{
    Q_OBJECT

public:
    SettingsDialogTab(SettingsDialog *dialog_,
                      std::function<SettingsPage *()> page_,
                      const QString &name, QString imageFileName,
                      SettingsTabId id);

    void setSelected(bool selected_);
    SettingsPage *page();
    SettingsTabId id() const;

    const QString &name() const;

signals:
    void selectedChanged(bool);

private:
    void paintEvent(QPaintEvent *) override;
    void mousePressEvent(QMouseEvent *event) override;

    struct {
        QString labelText;
        QIcon icon;
    } ui_;

    // Parent settings dialog
    SettingsDialog *dialog_{};
    SettingsPage *page_{};
    std::function<SettingsPage *()> lazyPage_;
    SettingsTabId id_;
    QString name_;

    bool selected_ = false;
};

}  // namespace chatterino
