// SPDX-FileCopyrightText: 2026 Contributors to Chatterino <https://chatterino.com>
//
// SPDX-License-Identifier: MIT

#pragma once

#ifdef CHATTERINO_HAVE_PLUGINS

#    include <pajlada/signals/scoped-connection.hpp>
#    include <QDialog>
#    include <QFont>
#    include <QPointer>
#    include <QString>
#    include <QVBoxLayout>

#    include <memory>

namespace chatterino {

struct RemotePlugin;
using RemotePluginPtr = std::shared_ptr<const RemotePlugin>;

class RemotePluginDialog : public QDialog
{
public:
    RemotePluginDialog(RemotePluginPtr remotePlugin, QWidget *parent = nullptr);
    ~RemotePluginDialog() override;

private:
    void appendField(const QString &name, const QString &value,
                     bool html = false);

    void refreshButtons();

    void togglePlugin();

    void installPlugin(bool update);
    void uninstallPlugin();

    void doUninstallPlugin(bool eraseData);

    QVBoxLayout vbox;
    QFont boldFont;
    RemotePluginPtr remotePlugin;

    QVBoxLayout buttonLayout;
    QPointer<QPushButton> installButton = nullptr;

    std::vector<pajlada::Signals::ScopedConnection> connections_;

    bool enabledRefreshQueued = false;
};

}  // namespace chatterino

#endif
