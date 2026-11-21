// SPDX-FileCopyrightText: 2018 Contributors to Chatterino <https://chatterino.com>
//
// SPDX-License-Identifier: MIT

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
    bool filterElements(const QString &query) override;

private:
    void tableCellClicked(const QModelIndex &clicked, EditableModelView *view);

    QStringListModel userListModel_;
    EditableModelView *view_;
};

}  // namespace chatterino
