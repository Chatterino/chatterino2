#pragma once
#include "common/Channel.hpp"
#include "providers/twitch/api/Helix.hpp"
#include "widgets/BaseWidget.hpp"

namespace Ui {
class StreamSettingsDialog;
}

namespace chatterino {

class StreamSettingsDialog : public QDialog
{
    Q_OBJECT

public:
    explicit StreamSettingsDialog(const ChannelPtr channel);
    ~StreamSettingsDialog();

protected slots:
    void updateGameSearch();
    void accept();

private:
    void reallyUpdateGameSearch();

    Ui::StreamSettingsDialog *ui_;

    QString originalTitle_;
    QString originalGame_;
    QString roomId_;
    QTimer lastUpdateTimer_;

    boost::optional<HelixGame> cachedGame_;
};

}  // namespace chatterino
