#pragma once

#include "widgets/helper/EditableModelView.hpp"
#include "widgets/settingspages/SettingsPage.hpp"

#include <QStringListModel>

class QVBoxLayout;

namespace chatterino {

class AliasesPage : public SettingsPage
{
public:
    AliasesPage();

private:
    void tableCellClicked(const QModelIndex &clicked, EditableModelView *view);

    QStringListModel userListModel_;
};

}  // namespace chatterino
