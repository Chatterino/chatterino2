#pragma once

#include "widgets/BaseWindow.hpp"
#include "widgets/TooltipEntry.hpp"

#include <pajlada/signals/signalholder.hpp>
#include <QLabel>
#include <QVBoxLayout>
#include <QWidget>

namespace chatterino {

class Image;
using ImagePtr = std::shared_ptr<Image>;

struct TooltipEntryRecord {
    ImagePtr image;
    QString text;
    int customWidth = 0;
    int customHeight = 0;
};

class TooltipWidget : public BaseWindow
{
    Q_OBJECT

public:
    static TooltipWidget *instance();

    TooltipWidget(BaseWidget *parent = nullptr);
    ~TooltipWidget() override = default;

    void setRecord(const TooltipEntryRecord &record);
    void setRecords(const std::vector<TooltipEntryRecord> &records);

    // void setText(QString text);
    void setWordWrap(bool wrap);
    void clearImage();
    // void setImage(ImagePtr image);
    // void setImageScale(int w, int h);

protected:
    void showEvent(QShowEvent *) override;
    void hideEvent(QHideEvent *) override;
    void changeEvent(QEvent *) override;
    void leaveEvent(QEvent *) override;
    void themeChangedEvent() override;
    void scaleChangedEvent(float) override;
    void paintEvent(QPaintEvent *) override;

private:
    void updateFont();

    void setVisibleEntries(int n);
    TooltipEntry *entryAt(int n);

    // set to true when tooltip image did not finish loading yet (pixmapOrLoad returned false)
    bool attemptRefresh{false};

    std::vector<TooltipEntry> entries_;
    int visibleEntries_ = 0;

    pajlada::Signals::SignalHolder connections_;

    QVBoxLayout *layout_;
};

}  // namespace chatterino
