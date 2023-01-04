#pragma once

#include "widgets/settingspages/SettingsPage.hpp"

#include <QAbstractTableModel>
#include <QTimer>

class QPushButton;
class QListWidget;

namespace chatterino {

class EditableModelView;

class HighlightingPage : public SettingsPage
{
public:
    HighlightingPage();

private:
    enum HighlightTab { Messages = 0, Users = 1, Badges = 2, Blacklist = 3 };

    QTimer disabledUsersChangedTimer_;

    void tableCellClicked(const QModelIndex &clicked, EditableModelView *view,
                          HighlightTab tab);
    void openSoundDialog(const QModelIndex &clicked, EditableModelView *view,
                         int soundColumn);
    void openColorDialog(const QModelIndex &clicked, EditableModelView *view,
                         HighlightTab tab);
};

}  // namespace chatterino
