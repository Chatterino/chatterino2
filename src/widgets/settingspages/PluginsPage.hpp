// SPDX-FileCopyrightText: 2023 Contributors to Chatterino <https://chatterino.com>
//
// SPDX-License-Identifier: MIT

#pragma once

#ifdef CHATTERINO_HAVE_PLUGINS
#    include "util/LayoutCreator.hpp"
#    include "widgets/settingspages/SettingsPage.hpp"

#    include <QDebug>
#    include <QFormLayout>
#    include <QGroupBox>
#    include <QWidget>

class QTableView;
class QPushButton;

namespace chatterino {
class Plugin;
class PluginRepository;

class PluginsPage : public SettingsPage
{
    Q_OBJECT

public:
    PluginsPage();
    ~PluginsPage() override;

private:
    void rebuildContent();

    void refreshRepositories();
    void appendRepository(PluginRepository &repo);

    void tableCellClicked(const QModelIndex &index);

    class RemoteTableModel;

    LayoutCreator<QWidget> scrollAreaWidget_;
    QGroupBox *generalGroup;
    QFrame *dataFrame_;

    RemoteTableModel *remoteTableModel;
    QTableView *remoteTableView;
    std::vector<std::shared_ptr<PluginRepository>> repositories;

    QString remoteWarnings;
    QPushButton *showRemoteWarningsButton;

    std::vector<pajlada::Signals::ScopedConnection> repoConnections;
};

}  // namespace chatterino

#endif
