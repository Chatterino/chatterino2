#pragma once
#include "common/Channel.hpp"
#include "providers/twitch/api/Helix.hpp"
#include "widgets/BaseWidget.hpp"

#include <QDialog>

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
    void queueLoadTags();

    void addTag();
    void removeTag();

private:
    void reallyLoadTags();
    void reallyLoadAllTags();
    void reallyUpdateGameSearch();
    void populateTagsAvailableList();

    Ui::StreamSettingsDialog *ui_;

    QString originalTitle_;
    QString originalGame_;

    QString roomId_;
    QTimer lastUpdateTimer_;

    std::vector<HelixTag> cachedTags_;
    bool isLoadingTags = false;
    bool needUpdateTags = false;
    QString lastTagsCursor_;
};

}  // namespace chatterino
