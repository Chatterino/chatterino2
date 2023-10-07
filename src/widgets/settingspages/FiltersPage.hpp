#pragma once

#include "widgets/settingspages/SettingsPage.hpp"

#include <QStringListModel>

class QVBoxLayout;

namespace chatterino {

class EditableModelView;

class FiltersPage : public SettingsPage
{
public:
    FiltersPage();

    void onShow() final;

private:
    void tableCellClicked(const QModelIndex &clicked, EditableModelView *view);

    QStringListModel userListModel_;
};

}  // namespace chatterino
