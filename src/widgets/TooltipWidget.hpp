#pragma once

#include "widgets/BaseWindow.hpp"
#include "widgets/TooltipEntryWidget.hpp"

#include <pajlada/signals/signalholder.hpp>
#include <QGridLayout>
#include <QLabel>
#include <QLayout>
#include <QVBoxLayout>
#include <QWidget>

namespace chatterino {

class Image;
using ImagePtr = std::shared_ptr<Image>;

struct TooltipEntry {
    ImagePtr image;
    QString text;
    int customWidth = 0;
    int customHeight = 0;
};

enum class TooltipStyle { Vertical, Grid };

class TooltipWidget : public BaseWindow
{
    Q_OBJECT

public:
    /// @param parent The parent of this widget.
    ///               If this widget is a window, use tooltipParentFor().
    ///               Remember to delete the widget!
    ///
    /// @see tooltipParentFor()
    TooltipWidget(BaseWidget *parent);
    ~TooltipWidget() override = default;

    void setOne(const TooltipEntry &entry,
                TooltipStyle style = TooltipStyle::Vertical);
    void set(const std::vector<TooltipEntry> &entries,
             TooltipStyle style = TooltipStyle::Vertical);

    void setWordWrap(bool wrap);
    void clearEntries();

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

    QLayout *currentLayout() const;
    int currentLayoutCount() const;
    TooltipEntryWidget *entryAt(int n);

    void setVisibleEntries(int n);
    void setCurrentStyle(TooltipStyle style);
    void addNewEntry(int absoluteIndex);

    void deleteCurrentLayout();
    void initializeVLayout();
    void initializeGLayout();

    int visibleEntries_ = 0;

    TooltipStyle currentStyle_;
    QVBoxLayout *vLayout_;
    QGridLayout *gLayout_;

    pajlada::Signals::SignalHolder connections_;
};

#ifdef Q_OS_WIN
template <typename T>
inline constexpr T *tooltipParentFor(T * /*desiredParent*/)
{
    return nullptr;
}
#else
template <typename T>
inline constexpr T *tooltipParentFor(T *desiredParent)
{
    return desiredParent;
}
#endif

}  // namespace chatterino
