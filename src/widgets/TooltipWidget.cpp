#include "widgets/TooltipWidget.hpp"

#include "Application.hpp"
#include "messages/Image.hpp"
#include "singletons/Fonts.hpp"
#include "singletons/WindowManager.hpp"

#include <QPainter>

namespace {

// number of columns in grid mode
constexpr int GRID_NUM_COLS = 3;

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

}  // namespace

namespace chatterino {

TooltipWidget::TooltipWidget(BaseWidget *parent)
    : BaseWindow({BaseWindow::TopMost, BaseWindow::DontFocus,
                  BaseWindow::DisableLayoutSave},
                 tooltipParentFor(parent))
{
    assert(parent != nullptr);
    QObject::connect(parent, &QObject::destroyed, this, &QObject::deleteLater);

    this->setStyleSheet("color: #fff; background: rgba(11, 11, 11, 0.8)");
    this->setAttribute(Qt::WA_TranslucentBackground);
    this->setWindowFlag(Qt::WindowStaysOnTopHint, true);

    // Default to using vertical layout
    this->initializeVLayout();
    this->setLayout(this->vLayout_);
    this->currentStyle_ = TooltipStyle::Vertical;

    this->connections_.managedConnect(getApp()->getFonts()->fontChanged,
                                      [this] {
                                          this->updateFont();
                                      });
    this->updateFont();

    auto *windows = getApp()->getWindows();
    this->connections_.managedConnect(windows->gifRepaintRequested, [this] {
        if (!this->isVisible())
        {
            return;
        }

        for (int i = 0; i < this->visibleEntries_; ++i)
        {
            auto *entry = this->entryAt(i);
            if (entry && entry->animated())
            {
                entry->refreshPixmap();
            }
        }
    });

    this->connections_.managedConnect(
        windows->layoutRequested, [this](auto *chan) {
            if (chan != nullptr || !this->isVisible())
            {
                return;
            }

            bool needSizeAdjustment = false;
            for (int i = 0; i < this->visibleEntries_; ++i)
            {
                auto *entry = this->entryAt(i);
                if (entry->hasImage() && entry->attemptRefresh())
                {
                    bool successfullyUpdated = entry->refreshPixmap();
                    needSizeAdjustment |= successfullyUpdated;
                }
            }

            if (needSizeAdjustment)
            {
                this->adjustSize();
                this->applyLastBoundsCheck();
            }
        });
}

void TooltipWidget::setOne(const TooltipEntry &entry, TooltipStyle style)
{
    this->set({entry}, style);
}

void TooltipWidget::set(const std::vector<TooltipEntry> &entries,
                        TooltipStyle style)
{
    this->setCurrentStyle(style);

    int delta = entries.size() - this->currentLayoutCount();
    if (delta > 0)
    {
        // Need to add more TooltipEntry instances
        int base = this->currentLayoutCount();
        for (int i = 0; i < delta; ++i)
        {
            this->addNewEntry(base + i);
        }
    }

    this->setVisibleEntries(entries.size());

    for (size_t i = 0; i < entries.size(); ++i)
    {
        if (auto *entryWidget = this->entryAt(static_cast<int>(i)))
        {
            const auto &entry = entries[i];
            entryWidget->setImage(entry.image);
            entryWidget->setText(entry.text);
            entryWidget->setImageScale(entry.customWidth, entry.customHeight);
        }
    }
    this->adjustSize();
}

void TooltipWidget::setVisibleEntries(int n)
{
    for (int i = 0; i < this->currentLayoutCount(); ++i)
    {
        auto *entry = this->entryAt(i);
        if (entry == nullptr)
        {
            continue;
        }

        if (i >= n)
        {
            entry->hide();
            entry->clearImage();
        }
        else
        {
            entry->show();
        }
    }
    this->visibleEntries_ = n;
}

void TooltipWidget::addNewEntry(int absoluteIndex)
{
    switch (this->currentStyle_)
    {
        case TooltipStyle::Vertical:
            this->vLayout_->addWidget(new TooltipEntryWidget(),
                                      Qt::AlignHCenter);
            return;
        case TooltipStyle::Grid:
            if (absoluteIndex == 0)
            {
                // Top row spans all columns
                this->gLayout_->addWidget(new TooltipEntryWidget(), 0, 0, 1,
                                          GRID_NUM_COLS, Qt::AlignCenter);
            }
            else
            {
                int row = ((absoluteIndex - 1) / GRID_NUM_COLS) + 1;
                int col = (absoluteIndex - 1) % GRID_NUM_COLS;
                this->gLayout_->addWidget(new TooltipEntryWidget(), row, col,
                                          Qt::AlignHCenter | Qt::AlignBottom);
            }
            return;
        default:
            return;
    }
}

// May be nullptr
QLayout *TooltipWidget::currentLayout() const
{
    switch (this->currentStyle_)
    {
        case TooltipStyle::Vertical:
            return this->vLayout_;
        case TooltipStyle::Grid:
            return this->gLayout_;
        default:
            return nullptr;
    }
}

int TooltipWidget::currentLayoutCount() const
{
    if (auto *layout = this->currentLayout())
    {
        return layout->count();
    }
    return 0;
}

// May be nullptr
TooltipEntryWidget *TooltipWidget::entryAt(int n)
{
    if (auto *layout = this->currentLayout())
    {
        return dynamic_cast<TooltipEntryWidget *>(layout->itemAt(n)->widget());
    }
    return nullptr;
}

void TooltipWidget::setCurrentStyle(TooltipStyle style)
{
    if (this->currentStyle_ == style)
    {
        // Nothing to update
        return;
    }

    this->clearEntries();
    this->deleteCurrentLayout();

    switch (style)
    {
        case TooltipStyle::Vertical:
            this->initializeVLayout();
            this->setLayout(this->vLayout_);
            break;
        case TooltipStyle::Grid:
            this->initializeGLayout();
            this->setLayout(this->gLayout_);
            break;
        default:
            break;
    }

    this->currentStyle_ = style;
}

void TooltipWidget::deleteCurrentLayout()
{
    auto *currentLayout = this->layout();
    delete currentLayout;

    switch (this->currentStyle_)
    {
        case TooltipStyle::Vertical:
            this->vLayout_ = nullptr;
            break;
        case TooltipStyle::Grid:
            this->gLayout_ = nullptr;
            break;
        default:
            break;
    }
}

void TooltipWidget::initializeVLayout()
{
    auto *vLayout = new QVBoxLayout(this);
    vLayout->setSizeConstraint(QLayout::SetFixedSize);
    vLayout->setContentsMargins(10, 5, 10, 5);
    vLayout->setSpacing(10);
    this->vLayout_ = vLayout;
}

void TooltipWidget::initializeGLayout()
{
    auto *gLayout = new QGridLayout(this);
    gLayout->setSizeConstraint(QLayout::SetFixedSize);
    gLayout->setContentsMargins(10, 5, 10, 5);
    gLayout->setHorizontalSpacing(8);
    gLayout->setVerticalSpacing(10);
    this->gLayout_ = gLayout;
}

void TooltipWidget::themeChangedEvent()
{
    //    this->setStyleSheet("color: #fff; background: #000");
}

void TooltipWidget::paintEvent(QPaintEvent *)
{
    QPainter painter(this);

    painter.fillRect(this->rect(), QColor(0, 0, 0, int(0.8 * 255)));
}

void TooltipWidget::scaleChangedEvent(float)
{
    this->updateFont();
}

void TooltipWidget::updateFont()
{
    this->setFont(getApp()->getFonts()->getFont(FontStyle::ChatMediumSmall,
                                                this->scale()));
}

void TooltipWidget::setWordWrap(bool wrap)
{
    for (int i = 0; i < this->visibleEntries_; ++i)
    {
        auto *entry = this->entryAt(i);
        if (entry)
        {
            entry->setWordWrap(wrap);
        }
    }
}

void TooltipWidget::clearEntries()
{
    this->setVisibleEntries(0);
}

void TooltipWidget::hideEvent(QHideEvent *)
{
    this->clearEntries();
}

void TooltipWidget::showEvent(QShowEvent *)
{
    this->adjustSize();
}

void TooltipWidget::changeEvent(QEvent *)
{
    // clear parents event
}

void TooltipWidget::leaveEvent(QEvent *)
{
    // clear parents event
}

}  // namespace chatterino
