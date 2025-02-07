#pragma once

#include "util/LayoutCreator.hpp"
#include "widgets/settingspages/SettingsPage.hpp"

#include <QStringListModel>

class QVBoxLayout;

namespace chatterino {

class EditableModelView;

class IgnoresPage : public SettingsPage
{
public:
    IgnoresPage();

    void onShow() final;

    bool filterElements(const QString &query) override;

private:
    QStringListModel userListModel_;
    QTabWidget *tabWidget_;
    EditableModelView *view_;

    void addPhrasesTab(LayoutCreator<QVBoxLayout> layout);
};

}  // namespace chatterino
